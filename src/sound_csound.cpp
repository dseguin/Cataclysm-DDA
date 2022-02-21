#if defined(CATA_CSOUND)

#include <algorithm>
#include <csound.h>
#include <sstream>
#include <string>
#include <vector>

#include "debug.h"
#include "filesystem.h"
#include "init.h"
#include "loading_ui.h"
#include "options.h"
#include "path_info.h"
#include "rng.h"
#include "sounds.h"
#include "sound_csound.h"

struct csound_skey {
    std::string id;
    std::string variant;
    std::string season;

    bool operator<( const csound_skey &rhs ) const {
        return std::tie( id, variant, season ) < std::tie( rhs.id, rhs.variant, rhs.season );
    }
};

struct csound_sval {
    void *lock = nullptr;
    CSOUND *sfx = nullptr;

    csound_sval( void *l, CSOUND *s ) : lock( l ), sfx( s ) { }
    csound_sval() : lock( nullptr ), sfx( nullptr ) { }
};

static std::string current_soundpack_path;
static std::map<csound_skey, std::vector<csound_sval>> effects;
// Thread sharing
static std::unique_ptr<std::vector<csound_sval *>> queued_effects =
            std::make_unique<std::vector<csound_sval *>>();
static int *csound_exit = nullptr;
static void *csound_thread = nullptr;
static void *queue_lock = nullptr;
static void *exit_lock = nullptr;

static uintptr_t async_callback( void * /*data*/ )
{
    bool quit = false;
    while( !quit ) {
        csound_sval *d = nullptr;
        csoundLockMutex( queue_lock );
        if( !queued_effects->empty() ) {
            d = queued_effects->back();
        }
        csoundUnlockMutex( queue_lock );
        if( !!d && !d->lock ) {
            csoundStart( d->sfx );
            while( !quit && !csoundPerformKsmps( d->sfx ) ) {
                csoundLockMutex( exit_lock );
                quit = !!*csound_exit;
                csoundUnlockMutex( exit_lock );
            }
            csoundSetScoreOffsetSeconds( d->sfx, 0 );
            csoundRewindScore( d->sfx );
            csoundLockMutex( queue_lock );
            for( auto iter = queued_effects->end() - 1; !queued_effects->empty() && *iter == d; iter-- ) {
                queued_effects->pop_back();
            }
            csoundUnlockMutex( queue_lock );
        }
        csoundLockMutex( exit_lock );
        quit = !!*csound_exit;
        csoundUnlockMutex( exit_lock );
    }
    return 0;
}

bool cata_csound::init_sound()
{
    csound_exit = static_cast<int *>( malloc( sizeof( *csound_exit ) ) );
    ( *csound_exit ) = 0;
    queued_effects->clear();
    csoundInitialize( 0 );
    queue_lock = csoundCreateThreadLock();
    exit_lock = csoundCreateThreadLock();
    csound_thread = csoundCreateThread( async_callback, nullptr );
    return true;
}

void cata_csound::shutdown_sound()
{
    csoundLockMutex( exit_lock );
    ( *csound_exit ) = 1;
    csoundUnlockMutex( exit_lock );
    csoundJoinThread( csound_thread );
    for( auto &sfx : effects ) {
        for( csound_sval &s : sfx.second ) {
            csoundDestroy( s.sfx );
        }
    }
    free( csound_exit );
    csound_exit = nullptr;
    csound_thread = nullptr;
    queue_lock = nullptr;
    exit_lock = nullptr;
    queued_effects->clear();
}

static csound_sval *find_random_effect( const csound_skey &id_var_seas )
{
    auto iter = effects.find( id_var_seas );
    if( iter == effects.end() || iter->second.empty() ) {
        return nullptr;
    }
    auto skey = iter->second.begin();
    std::advance( skey, rng( 0, iter->second.size() - 1 ) );
    return &*skey;
}

static csound_sval *find_random_effect( const std::string &id, const std::string &variant,
                                        const std::string &season )
{
    csound_sval *eff1 = find_random_effect( { id, variant, season } );
    if( eff1 != nullptr ) {
        return eff1;
    }
    csound_sval *eff2 = find_random_effect( { id, variant, "" } );
    if( eff2 != nullptr ) {
        return eff2;
    }
    csound_sval *eff3 = find_random_effect( { id, "default", season } );
    if( eff3 != nullptr ) {
        return eff3;
    }
    return find_random_effect( { id, "default", "" } );
}

bool cata_csound::has_variant_sfx( const std::string &id, const std::string &variant,
                                   const std::string &season )
{
    return find_random_effect( id, variant, season ) != nullptr;
}

void cata_csound::play_variant_sfx( const std::string &id, const std::string &variant,
                                    const std::string &season, int /*volume*/ )
{
    csound_sval *sfx = find_random_effect( id, variant, season );
    if( !!sfx ) {
        csoundLockMutex( queue_lock );
        queued_effects->insert( queued_effects->begin(), sfx );
        csoundRewindScore( sfx->sfx );
        csoundUnlockMutex( queue_lock );
    }
}

void cata_csound::play_variant_sfx( const std::string &id, const std::string &variant,
                                    const std::string &season, int volume, units::angle /*angle*/, double /*pitch_min*/,
                                    double /*pitch_max*/ )
{
    play_variant_sfx( id, variant, season, volume );
}

void cata_csound::play_ambient_variant_sfx( const std::string &id, const std::string &variant,
        const std::string &season, int volume, sfx::channel /*channel*/, int /*fade_in_duration*/,
        double /*pitch*/, int /*loops*/ )
{
    play_variant_sfx( id, variant, season, volume );
}

static bool load_csd( const std::string &csd_file, const csound_skey &sound )
{
    if( !csd_file.empty() ) {
        std::ifstream f( fs::u8path( csd_file ), std::ios::in );
        if( !f ) {
            DebugLog( D_ERROR, D_CSOUND ) << "Could not load CSD file: " << csd_file;
            return false;
        }
    } else {
        return false;
    }

    std::string data = read_entire_file( csd_file );
    CSOUND *sfx = csoundCreate( nullptr );
    if( int res = csoundCompileCsdText( sfx, data.c_str() ) ) {
        DebugLog( D_ERROR, D_CSOUND ) << "Error compiling CSD file: " << csd_file << " (" << res << ")";
        return false;
    }
    effects[sound].emplace_back( nullptr, sfx );
    return true;
}

void cata_csound::load_sfx( const std::array<std::string, 3> &id_variant_season, int /*volume*/,
                            const std::vector<std::string> &files )
{
    csound_skey sound;
    sound.id = id_variant_season[0];
    sound.variant = id_variant_season[1];
    sound.season = id_variant_season[2];

    csoundSetGlobalEnv( "INCDIR", current_soundpack_path.c_str() );

    for( const std::string &file : files ) {
        load_csd( current_soundpack_path + "/" + file, sound );
    }
}

void cata_csound::load_soundset()
{
    const std::string default_path = PATH_INFO::defaultcsounddir();
    const std::string default_soundpack = "basic";
    std::string current_soundpack = get_option<std::string>( "CSOUNDPACKS" );
    std::string soundpack_path;

    if( current_soundpack.empty() ) {
        DebugLog( D_ERROR, D_CSOUND ) << "Soundpack not set in options or empty.";
        current_soundpack = default_soundpack;
        soundpack_path = default_path;
    } else {
        DebugLog( D_INFO, D_CSOUND ) << "Current soundpack is: " << current_soundpack;
        soundpack_path = CSOUNDPACKS[current_soundpack];
    }

    if( soundpack_path.empty() ) {
        DebugLog( D_ERROR, D_CSOUND ) << "Soundpack with name " << current_soundpack <<
                                      " can't be found or empty string";
        soundpack_path = default_path;
        current_soundpack = default_soundpack;
    } else {
        DebugLog( D_INFO, D_CSOUND ) << '"' << current_soundpack << '"' << " soundpack: found path: " <<
                                     soundpack_path;
    }

    current_soundpack_path = soundpack_path;
    try {
        loading_ui ui( false );
        DynamicDataLoader::get_instance().load_data_from_path( soundpack_path, "core", ui );
    } catch( const std::exception &err ) {
        DebugLog( D_ERROR, D_CSOUND ) << "failed to load sounds: " << err.what();
    }
}

#endif // CATA_CSOUND