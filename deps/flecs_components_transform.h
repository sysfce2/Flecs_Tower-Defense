// Comment out this line when using as DLL
#define flecs_components_transform_STATIC
#ifndef FLECS_COMPONENTS_TRANSFORM_H
#define FLECS_COMPONENTS_TRANSFORM_H

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

#ifndef FLECS_COMPONENTS_TRANSFORM_BAKE_CONFIG_H
#define FLECS_COMPONENTS_TRANSFORM_BAKE_CONFIG_H

/* Headers of public dependencies */
#include "flecs.h"
#include "flecs_components_cglm.h"

/* Convenience macro for exporting symbols */
#ifndef flecs_components_transform_STATIC
#if flecs_components_transform_EXPORTS && (defined(_MSC_VER) || defined(__MINGW32__))
  #define FLECS_COMPONENTS_TRANSFORM_API __declspec(dllexport)
#elif flecs_components_transform_EXPORTS
  #define FLECS_COMPONENTS_TRANSFORM_API __attribute__((__visibility__("default")))
#elif defined _MSC_VER
  #define FLECS_COMPONENTS_TRANSFORM_API __declspec(dllimport)
#else
  #define FLECS_COMPONENTS_TRANSFORM_API
#endif
#else
  #define FLECS_COMPONENTS_TRANSFORM_API
#endif

#endif



// Reflection system boilerplate
#undef ECS_META_IMPL
#ifndef FLECS_COMPONENTS_TRANSFORM_IMPL
#define ECS_META_IMPL EXTERN // Ensure meta symbols are only defined once
#endif

#ifndef FLECS_LEGACY

FLECS_COMPONENTS_TRANSFORM_API
ECS_STRUCT(EcsPosition2, {
    float x;
    float y;
});

FLECS_COMPONENTS_TRANSFORM_API
ECS_STRUCT(EcsPosition3, {
    float x;
    float y;
    float z;
});

FLECS_COMPONENTS_TRANSFORM_API
ECS_STRUCT(EcsScale2, {
    float x;
    float y;
});

FLECS_COMPONENTS_TRANSFORM_API
ECS_STRUCT(EcsScale3, {
    float x;
    float y;
    float z;
});

FLECS_COMPONENTS_TRANSFORM_API
ECS_STRUCT(EcsRotation2, {
    float angle;
});

FLECS_COMPONENTS_TRANSFORM_API
ECS_STRUCT(EcsRotation3, {
    float x;
    float y;
    float z;
});

FLECS_COMPONENTS_TRANSFORM_API
ECS_STRUCT(EcsQuaternion, {
    float x;
    float y;
    float z;
    float w;
});

#endif

typedef struct EcsTransform2 {
    mat3 value;
} EcsTransform2;

typedef struct EcsTransform3 {
    mat4 value;
} EcsTransform3;

typedef struct EcsProject2 {
    mat3 value;
} EcsProject2;

typedef struct EcsProject3 {
    mat4 value;
} EcsProject3;

#ifdef __cplusplus
extern "C" {
#endif

FLECS_COMPONENTS_TRANSFORM_API
void FlecsComponentsTransformImport(
    ecs_world_t *world);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
#ifndef FLECS_NO_CPP

namespace flecs {
namespace components {

class transform {
public:
    using Position2 = EcsPosition2;

    struct Position3 : EcsPosition3 {
        Position3() { }
        
        Position3(float x, float y, float z) {
            this->x = x;
            this->y = y;
            this->z = z;
        }

        operator float*() {
            return reinterpret_cast<float*>(this);
        }
    };

    using Scale2 = EcsScale2;
    using Scale3 = EcsScale3;

    using Rotation2 = EcsRotation2;
    using Rotation3 = EcsRotation3;

    using Quaternion = EcsQuaternion;

    using Transform2 = EcsTransform2;
    using Transform3 = EcsTransform3;

    using Project2 = EcsProject2;
    using Project3 = EcsProject3;

    transform(flecs::world& ecs) {
        // Load module contents
        FlecsComponentsTransformImport(ecs);

        // Bind C++ types with module contents
        ecs.module<flecs::components::transform>();
        ecs.component<Position2>();
        ecs.component<Position3>();
        ecs.component<Scale2>();
        ecs.component<Scale3>();
        ecs.component<Rotation2>();
        ecs.component<Rotation3>();
        ecs.component<Quaternion>();
        ecs.component<Transform2>();
        ecs.component<Transform3>();
        ecs.component<Project2>();
        ecs.component<Project3>();
    }
};

}
}

#endif // FLECS_NO_CPP
#endif // __cplusplus

#endif // FLECS_COMPONENTS_TRANSFORM_H

