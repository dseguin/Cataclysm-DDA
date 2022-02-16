#pragma once
#ifndef CATA_SRC_SOUND_CSOUND_H
#define CATA_SRC_SOUND_CSOUND_H

#include <string>

#include "sounds.h"

namespace cata_csound
{
#if defined(CATA_CSOUND)

/**
 * Attempt to initialize an audio device.  Returns false if initialization fails.
 */
bool init_sound();
void shutdown_sound();
void play_music( const std::string &playlist );
void stop_music();
void update_music_volume();
void load_soundset();
void load_sfx( const std::array<std::string, 3> &id_variant_season, int volume,
               const std::vector<std::string> &files );
bool has_variant_sfx( const std::string &id, const std::string &variant,
                      const std::string &season );
void play_variant_sfx( const std::string &id, const std::string &variant,
                       const std::string &season, int volume );
void play_variant_sfx( const std::string &id, const std::string &variant,
                       const std::string &season, int volume,
                       units::angle angle, double pitch_min, double pitch_max );
void play_ambient_variant_sfx( const std::string &id, const std::string &variant,
                               const std::string &season, int volume,
                               sfx::channel channel, int fade_in_duration, double pitch, int loops );

#else

inline bool init_sound()
{
    return false;
}
inline void shutdown_sound() { }
inline void play_music( const std::string &/*playlist*/ ) { }
inline void update_music_volume() { }
inline void load_soundset() { }
inline void load_sfx( const std::array<std::string, 3> &/*id_variant_season*/, int /*volume*/,
                      const std::vector<std::string> &/*files*/ ) { }
inline bool has_variant_sfx( const std::string &/*id*/, const std::string &/*variant*/,
                             const std::string &/*season*/ );
inline void play_variant_sfx( const std::string &id, const std::string &variant,
                              const std::string &season, int volume ) { }
inline void play_variant_sfx( const std::string &id, const std::string &variant,
                              const std::string &season, int volume,
                              units::angle angle, double pitch_min, double pitch_max ) { }
inline void play_ambient_variant_sfx( const std::string &id, const std::string &variant,
                                      const std::string &season, int volume,
                                      sfx::channel channel, int fade_in_duration, double pitch, int loops ) { }

#endif
}

#endif // CATA_SRC_SOUND_CSOUND_H