// Comment out this line when using as DLL
#define flecs_systems_transform_STATIC
#ifndef FLECS_SYSTEMS_TRANSFORM_H
#define FLECS_SYSTEMS_TRANSFORM_H

/*
                                   )
                                  (.)
                                  .|.
                                  | |
                              _.--| |--._
                           .-';  ;`-'& ; `&.
                          \   &  ;    &   &_/
                           |"""---...---"""|
                           \ | | | | | | | /
                            `---.|.|.|.---'

 * This file is generated by bake.lang.c for your convenience. Headers of
 * dependencies will automatically show up in this file. Include bake_config.h
 * in your main project file. Do not edit! */

#ifndef FLECS_SYSTEMS_TRANSFORM_BAKE_CONFIG_H
#define FLECS_SYSTEMS_TRANSFORM_BAKE_CONFIG_H

/* Headers of public dependencies */
#include "flecs.h"
#include "cglm.h"
#include "flecs_components_transform.h"

/* Convenience macro for exporting symbols */
#ifndef flecs_systems_transform_STATIC
#if flecs_systems_transform_EXPORTS && (defined(_MSC_VER) || defined(__MINGW32__))
  #define FLECS_SYSTEMS_TRANSFORM_API __declspec(dllexport)
#elif flecs_systems_transform_EXPORTS
  #define FLECS_SYSTEMS_TRANSFORM_API __attribute__((__visibility__("default")))
#elif defined _MSC_VER
  #define FLECS_SYSTEMS_TRANSFORM_API __declspec(dllimport)
#else
  #define FLECS_SYSTEMS_TRANSFORM_API
#endif
#else
  #define FLECS_SYSTEMS_TRANSFORM_API
#endif

#endif



#ifdef __cplusplus
extern "C" {
#endif

FLECS_SYSTEMS_TRANSFORM_API
void FlecsSystemsTransformImport(
    ecs_world_t *world);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
#ifndef FLECS_NO_CPP

namespace flecs {
namespace systems {

class transform {
public:
    transform(flecs::world& ecs) {
        // Load module contents
        FlecsSystemsTransformImport(ecs);

        // Bind module contents with C++ types
        ecs.module<flecs::systems::transform>();
    }
};

}
}

#endif // FLECS_NO_CPP
#endif // __cplusplus

#endif

