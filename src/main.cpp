#include <tower_defense.h>
#include <vector>
#include <iostream>

using namespace flecs::components;

// Shortcuts to imported components
using Position = transform::Position3;
using Rotation = transform::Rotation3;
using Velocity = physics::Velocity3;
using Input = input::Input;
using SpatialQuery = flecs::systems::physics::SpatialQuery;
using SpatialQueryResult = flecs::systems::physics::SpatialQueryResult;
using Color = graphics::Rgb;
using Specular = graphics::Specular;
using Emissive = graphics::Emissive;
using Box = geometry::Box;

#define ECS_PI_2 ((float)(GLM_PI * 2))

// Game constants
static const float EnemySize = 0.7;
static const float EnemySpeed = 5.0;
static const float EnemySpawnInterval = 0.25;

static const float TurretRotateSpeed = 4.0;
static const float TurretFireInterval = 0.1;
static const float TurretRange = 5.0;
static const float TurretCannonOffset = 0.2;
static const float TurretCannonLength = 0.6;

static const float BulletSize = 0.1;
static const float BulletSpeed = 24.0;
static const float BulletLifespan = 0.5;
static const float BulletDamage = 0.007;

static const float IonSize = 0.07;
static const float IonLifespan = 1.5;
static const float IonDecay = 0.1;

static const float BeamFireInterval = 0.1;
static const float BeamDamage = 0.5;
static const float BeamSize = 0.2;

static const float FireballSize = 0.3;
static const float FireballSizeDecay = 0.0001;
static const float FireballLifespan = 0.2;

static const float BoltSize = 0.6;
static const float BoltSizeDecay = 0.01;
static const float BoltLifespan = 5.3;

static const float SmokeSize = 1.5;
static const float SmokeRadius = 1.2;
static const float SmokeSizeDecay = 0.4;
static const float SmokeColorDecay = 0.01;
static const float SmokeLifespan = 4.0;

static const float SparkSize = 0.20;
static const float SparkLifespan = 0.5;
static const float SparkSizeDecay = 0.05;
static const float SparkVelocityDecay = 0.05;
static const float SparkInitialVelocity = 6.0;

static const float TileSize = 3.0;
static const float TileHeight = 0.5;
static const float PathHeight = 0.1;
static const float TileSpacing = 0.00;
static const int TileCountX = 10;
static const int TileCountZ = 10;

// Direction vector. During pathfinding enemies will cycle through this vector
// to find the next direction to turn to.
static const transform::Position2 dir[] = {
    {-1, 0},
    {0, -1},
    {1, 0},
    {0, 1},
};

// Grid wrapper around a vector which translates an (x, y) into a vector index
template <typename T>
class grid {
public:
    grid(int width, int height)
        : m_width(width)
        , m_height(height) 
    { 
        for (int x = 0; x < width; x ++) {
            for (int y = 0; y < height; y ++) {
                m_values.push_back(T());
            }
        }
    }

    void set(int32_t x, int32_t y, T value) {
        m_values[y * m_width + x] = value;
    }

    const T operator()(int32_t x, int32_t y) {
        return m_values[y * m_width + x];
    }

private:
    int m_width;
    int m_height;
    std::vector<T> m_values;
};

// Components

struct Game {
    flecs::entity window;
    flecs::entity level;
    
    Position center;
    float size;        
};

struct Level {
    Level() {
        map = nullptr;
    }

    Level(grid<bool> *arg_map, transform::Position2 arg_spawn) {
        map = arg_map;
        spawn_point = arg_spawn;
    }

    grid<bool> *map;
    transform::Position2 spawn_point;  
};

struct Particle {
    float size_decay;
    float color_decay;
    float velocity_decay;
    float lifespan;
};

struct ParticleLifespan {
    ParticleLifespan() {
        t = 0;
    }
    float t;
};

struct Enemy { };

struct Direction {
    int value;
};

struct Health {
    Health() {
        value = 1;
    }
    float value;
};

struct Bullet { };

struct Turret { 
    Turret(float fire_interval_arg = 1.0) {
        lr = 1;
        t_since_fire = 0;
        fire_interval = fire_interval_arg;
        beam_countdown = 0;
    }

    float fire_interval;
    float t_since_fire;
    float beam_countdown;
    int lr;
};

struct Railgun { };

struct Target {
    Target() {
        prev_position[0] = 0;
        prev_position[1] = 0;
        prev_position[2] = 0;
        lock = false;
    }

    flecs::entity target;
    vec3 prev_position;
    vec3 aim_position;
    float angle;
    float distance;
    bool lock;
};

// Prefab types
namespace prefabs {
    struct Tree {
        struct Trunk { };
        struct Canopy { };
    };

    struct Tile { };
    struct Path { };
    struct Enemy { };

    struct materials {
        struct Beam { };
        struct Metal { };
        struct CannonHead { };
        struct RailgunLight { };
    };

    struct Particle { };
    struct Bullet { };
    struct Fireball { };
    struct Smoke { };
    struct Spark { };
    struct Ion { };
    struct Bolt { };

    struct Turret {
        struct Base { };
        struct Head { };
    };

    struct Cannon {
        struct Head {
            struct BarrelLeft { };
            struct BarrelRight { };
        };
        struct Barrel { };
    };

    struct Railgun {
        struct Head {
            struct Beam { };
        };
    };
}

// Scope for systems
struct systems { };

// Scope for level entities (tile, path)
struct level { };

// Scope for turrets
struct turrets { };

// Scope for enemies
struct enemies { };

// Utility functions
float randf(float scale) {
    return ((float)rand() / (float)RAND_MAX) * scale;
}

float to_coord(float x) {
    return x * (TileSpacing + TileSize) - (TileSize / 2.0);
}

float from_coord(float x) {
    return (x + (TileSize / 2.0)) / (TileSpacing + TileSize);
}

float to_x(float x) {
    return to_coord(x + 0.5) - to_coord((TileCountX / 2.0));
}

float to_z(float z) {
    return to_coord(z);
}

float from_x(float x) {
    return from_coord(x + to_coord((TileCountX / 2.0))) - 0.5;
}

float from_z(float z) {
    return from_coord(z);
}

float angle_normalize(float angle) {
    return angle - floor(angle / ECS_PI_2) * ECS_PI_2;
}

float look_at(vec3 eye, vec3 dest) {
    vec3 diff;
    
    glm_vec3_sub(dest, eye, diff);
    float x = fabs(diff[0]), z = fabs(diff[2]);
    bool x_sign = diff[0] < 0, z_sign = diff[2] < 0;
    float r = atan(z / x);

    if (z_sign) {
        r += GLM_PI;
    }

    if (z_sign == x_sign) {
        r = -r + GLM_PI;
    }

    return angle_normalize(r + GLM_PI);
}

float rotate_to(float cur, float target, float increment) {
    cur = angle_normalize(cur);
    target = angle_normalize(target);

    if (cur - target > GLM_PI) {
        cur -= ECS_PI_2;
    } else if (cur - target < -GLM_PI) {
        cur += ECS_PI_2;
    }
    
    if (cur > target) {
        cur -= increment;
        if (cur < target) {
            cur = target;
        }
    } else {
        cur += increment;
        if (cur > target) {
            cur = target;
        }
    }

    return cur;
}

// Check if enemy needs to change direction
bool find_path(Position& p, Direction& d, const Level* lvl) {
    // Check if enemy is in center of tile
    float t_x = from_x(p.x);
    float t_y = from_z(p.z);
    int ti_x = (int)t_x;
    int ti_y = (int)t_y;
    float td_x = t_x - ti_x;
    float td_y = t_y - ti_y;

    // If enemy is in center of tile, decide where to go next
    if (td_x < 0.1 && td_y < 0.1) {
        grid<bool> *tiles = lvl->map;

        // Compute backwards direction so we won't try to go there
        int backwards = (d.value + 2) % 4;

        // Find a direction that the enemy can move to
        for (int i = 0; i < 3; i ++) {
            int n_x = ti_x + dir[d.value].x;
            int n_y = ti_y + dir[d.value].y;

            if (n_x >= 0 && n_x <= TileCountX) {
                if (n_y >= 0 && n_y <= TileCountZ) {
                    // Next tile is still on the grid, test if it's a path
                    if (tiles[0](n_x, n_y)) {
                        // Next tile is a path, so continue along current direction
                        return false;
                    }
                }
            }

            // Try next direction. Make sure not to move backwards
            do {
                d.value = (d.value + 1) % 4;
            } while (d.value == backwards);
        }

        // If enemy was not able to find a next direction, it reached the end
        return true;        
    }

    return false;
}

void SpawnEnemy(flecs::iter& it, const Game *g) {
    const Level* lvl = g->level.get<Level>();

    it.world().entity().child_of<enemies>().is_a<prefabs::Enemy>()
        .set<Direction>({0})
        .set<Position>({
            lvl->spawn_point.x, -1.2, lvl->spawn_point.y
        });
}

void MoveEnemy(flecs::iter& it, size_t i,
    Position& p, Direction& d, const Game& g)
{
    const Level* lvl = g.level.get<Level>();

    if (find_path(p, d, lvl)) {
        it.entity(i).destruct();
    } else {
        p.x += dir[d.value].x * EnemySpeed * it.delta_time();
        p.z += dir[d.value].y * EnemySpeed * it.delta_time();
    }
}

void ClearTarget(Target& target, Position& p) {
    flecs::entity t = target.target;
    if (t) {
        if (!t.is_alive()) {
            target.target = flecs::entity::null();
            target.lock = false;
        } else {
            Position target_pos = t.get<Position>()[0];
            float distance = glm_vec3_distance(p, target_pos);
            if (distance > TurretRange) {
                target.target = flecs::entity::null();
                target.lock = false;
            }
        }
    }
}

void FindTarget(flecs::iter& it, size_t i,
    Turret& turret, Target& target, Position& p) 
{
    auto q = it.field<const SpatialQuery>(4);
    auto qr = it.field<SpatialQueryResult>(5);

    if (target.target) {
        return;
    }

    flecs::entity enemy;
    float distance = 0, min_distance = 0;

    q->findn(p, TurretRange, qr[i]);
    for (auto e : qr[i]) {
        distance = glm_vec3_distance(p, e.pos);
        if (distance > TurretRange) {
            continue;
        }

        if (!min_distance || distance < min_distance) {
            min_distance = distance;
            enemy = it.world().entity(e.id);
        }
    }

    if (min_distance) {
        target.target = enemy;
        target.distance = min_distance;
    }
}

void AimTarget(flecs::iter& it, size_t i,
    Turret& turret, Target& target, Position& p) 
{
    flecs::entity enemy = target.target;
    if (enemy && enemy.is_alive()) {
        flecs::entity e = it.entity(i);

        Position target_p = enemy.get<Position>()[0];
        vec3 diff;
        glm_vec3_sub(target_p, target.prev_position, diff);

        target.prev_position[0] = target_p.x;
        target.prev_position[1] = target_p.y;
        target.prev_position[2] = target_p.z;
        float distance = glm_vec3_distance(p, target_p);

        // Crude correction for enemy movement and bullet travel time
        flecs::entity beam = e.target<prefabs::Railgun::Head::Beam>();
        if (!beam) {
            glm_vec3_scale(diff, distance * 5, diff);
            glm_vec3_add(target_p, diff, target_p);
        }

        target.aim_position[0] = target_p.x;
        target.aim_position[1] = target_p.y;
        target.aim_position[2] = target_p.z;            

        float angle = look_at(p, target_p);

        flecs::entity head = e.target<prefabs::Turret::Head>();
        Rotation r = head.get<Rotation>()[0];
        r.y = rotate_to(r.y, angle, TurretRotateSpeed * it.delta_time());
        head.set<Rotation>(r);
        target.angle = angle;
        target.lock = (r.y == angle) * (distance < TurretRange);
    }
}

void FireCountdown(flecs::iter& it, size_t i,
    Turret& turret, Target& target) 
{
    turret.t_since_fire += it.delta_time();
    if (turret.beam_countdown > 0) {
        turret.beam_countdown -= it.delta_time();
        if (turret.beam_countdown <= 0 || !target.lock) {
            it.entity(i).target<prefabs::Railgun::Head::Beam>().disable();
            turret.t_since_fire = 0;
        }
    }
}

void BeamControl(flecs::iter& it, size_t i,
    Position& p, Turret& turret, Target& target, const Game& g) 
{
    flecs::entity beam = it.entity(i).target<prefabs::Railgun::Head::Beam>();
    if (target.lock && beam && beam.enabled()) {
        flecs::entity enemy = target.target;
        if (!enemy.is_alive()) {
            return;
        }

        // Position beam at enemy
        Position pos = p;
        Position target_pos = enemy.get<Position>()[0];
        float distance = glm_vec3_distance(p, target_pos);
        beam.set<Position>({ (distance / 2), 0.0, 0.0 });
        beam.set<Box>({0.06, 0.06, distance});

        // Subtract health from enemy as long as beam is firing
        enemy.get([&](Health& h) {
            h.value -= BeamDamage * it.delta_time();
        });

        // Create ion trail
        if (randf(1.0) > 0.3) {
            vec3 v;
            glm_vec3_sub(pos, target_pos, v);
            glm_vec3_normalize(v);

            float ion_d = randf(distance - 0.7) + 0.7;
            Position ion_pos = {pos.x - ion_d * v[0], -1.2, pos.z - ion_d * v[2]};
            Velocity ion_v = {
                randf(0.05),
                randf(0.05),
                randf(0.05)
            };

            it.world().entity().is_a<prefabs::Ion>()
                .set<Position>(ion_pos)
                .set<Velocity>(ion_v);
        }
    }
}

void FireAtTarget(flecs::iter& it, size_t i,
    Turret& turret, Target& target, Position& p, const Game& g)
{
    auto ecs = it.world();
    bool is_railgun = it.is_set(5);

    if (turret.t_since_fire < turret.fire_interval) {
        return;
    }

    if (target.target && target.lock) {
        Position pos = p;
        float angle = target.angle;
        vec3 v, target_p;
        target_p[0] = target.aim_position[0];
        target_p[1] = target.aim_position[1];
        target_p[2] = target.aim_position[2];
        glm_vec3_sub(p, target_p, v);
        glm_vec3_normalize(v);

        if (!is_railgun) {
            pos.x += 1.8 * TurretCannonLength * -v[0];
            pos.z += 1.8 * TurretCannonLength * -v[2];
            glm_vec3_scale(v, BulletSpeed, v);
            pos.x += sin(angle) * TurretCannonOffset * turret.lr;
            pos.y = -1.2;
            pos.z += cos(angle) * TurretCannonOffset * turret.lr;
            turret.lr = -turret.lr;
            ecs.entity().is_a<prefabs::Bullet>()
                .set<Position>(pos)
                .set<Velocity>({-v[0], 0, -v[2]});
            ecs.entity().is_a<prefabs::Fireball>()
                .set<Position>(pos)
                .set<Rotation>({0, angle, 0});  
        } else if (turret.beam_countdown <= 0) {
            it.entity(i).target<prefabs::Railgun::Head::Beam>().enable();
            turret.beam_countdown = 1.0;
            pos.x += 1.2 * -v[0];
            pos.y = -1.2;
            pos.z += 1.2 * -v[2];
            ecs.entity().is_a<prefabs::Bolt>()
                .set<Position>(pos)
                .set<Rotation>({0, angle, 0}); 
        }

        turret.t_since_fire = 0;
    }
}

void ProgressParticle(flecs::iter& it,
    ParticleLifespan *pl, const Particle *p, Box *box, Color *color, Velocity *vel)
{
    if (it.is_set(3)) {
        for (auto i : it) {
            box[i].width *= pow(p->size_decay, it.delta_time());
            box[i].height *= pow(p->size_decay, it.delta_time());
            box[i].depth *= pow(p->size_decay, it.delta_time());
        }
    }
    if (it.is_set(4)) {
        for (auto i : it) {
            color[i].r *= pow(p->color_decay, it.delta_time());
            color[i].g *= pow(p->color_decay, it.delta_time());
            color[i].b *= pow(p->color_decay, it.delta_time());
        }        
    }
    if (it.is_set(5)) {
        for (auto i : it) {
            vel[i].x *= pow(p->velocity_decay, it.delta_time());
            vel[i].y *= pow(p->velocity_decay, it.delta_time());
            vel[i].z *= pow(p->velocity_decay, it.delta_time());
        }          
    }
    for (auto i : it) {
        pl[i].t += it.delta_time();
        if (pl[i].t > p->lifespan) {
            it.entity(i).destruct();
        }
    }
}

void HitTarget(flecs::iter& it, size_t i,
    Position& p, Health& h, Box& b)
{
    auto q = it.field<const SpatialQuery>(4);
    auto qr = it.field<SpatialQueryResult>(5);
    
    float range = b.width / 2;

    q->findn(p, range, qr[i]);
    for (auto e : qr[i]) {
        it.world().entity(e.id).destruct();
        h.value -= BulletDamage;
    }
}

static
void explode(flecs::world& ecs, const Game& g, Position& p) {
    for (int s = 0; s < 25; s ++) {
        float red = randf(0.5) + 0.7;
        float green = randf(0.5);
        float blue = randf(0.3);
        float size = SmokeSize * randf(1.0);
        if (green > red) {
            green = red;
        }
        if (blue > green) {
            blue = green;
        }

        ecs.entity().is_a<prefabs::Smoke>()
            .set<Position>({
                p.x + randf(SmokeRadius) - SmokeRadius / 2, 
                p.y + randf(SmokeRadius) - SmokeRadius / 2,  
                p.z + randf(SmokeRadius) - SmokeRadius / 2})
            .set<Box>({size, size, size})
            .set<Color>({red, green, blue});
    }
    for (int s = 0; s < 15; s ++) {
        float x_r = randf(ECS_PI_2);
        float y_r = randf(ECS_PI_2);
        float z_r = randf(ECS_PI_2);
        float speed = randf(SparkInitialVelocity) + 2.0;
        ecs.entity().is_a<prefabs::Spark>()
            .set<Position>({p.x, p.y, p.z}) 
            .set<Velocity>({
                cos(x_r) * speed, cos(y_r) * speed, cos(z_r) * speed});
    }
}

void DestroyEnemy(flecs::entity e,
    Health& h, Position& p, const Game& g, const graphics::Camera& cam) 
{
    flecs::world ecs = e.world();
    if (h.value <= 0) {
        e.destruct();
        explode(ecs, g, p);
    }
}

void UpdateEnemyColor(Color& c, Health& h) {
    c.r = (1.0 - h.value) / 2;
    c.g = (1.0 - h.value) / 5;
    c.b = (1.0 - h.value) / 7;
}

void init_game(flecs::world& ecs) {
    Game *g = ecs.get_mut<Game>();
    g->center = { to_x(TileCountX / 2), 0, to_z(TileCountZ / 2) };
    g->size = TileCountX * (TileSize + TileSpacing) + 2;
}

void init_ui(flecs::world& ecs) {
    graphics::Camera camera_data = {};
    camera_data.set_up(0, -1, 0);
    camera_data.set_fov(20);
    camera_data.near_ = 1.0;
    camera_data.far_ = 80.0;
    auto camera = ecs.entity("Camera")
        .add(flecs::game::CameraController)
        .set<Position>({0, -9.0, -10.0})
        .set<Rotation>({0.4})
        .set<graphics::Camera>(camera_data);

    graphics::DirectionalLight light_data = {};
    light_data.set_direction(0.3, -1.0, 0.5);
    light_data.set_color(0.95, 0.90, 0.75);
    auto light = ecs.entity("Sun")
        .set<graphics::DirectionalLight>(light_data);

    gui::Canvas canvas_data = {};
    canvas_data.width = 1200;
    canvas_data.height = 900;
    canvas_data.title = (char*)"Flecs Tower Defense";
    canvas_data.background_color = {0.60, 0.65, 0.8};
    canvas_data.ambient_light = {0.03, 0.06, 0.09};
    canvas_data.camera = camera.id();
    canvas_data.directional_light = light.id();
    ecs.entity().set<gui::Canvas>(canvas_data);
}

// Init level
void init_level(flecs::world& ecs) {
    Game *g = ecs.get_mut<Game>();

    grid<bool> *path = new grid<bool>(TileCountX, TileCountZ);
    path->set(0, 1, true); path->set(1, 1, true); path->set(2, 1, true);
    path->set(3, 1, true); path->set(4, 1, true); path->set(5, 1, true);
    path->set(6, 1, true); path->set(7, 1, true); path->set(8, 1, true);
    path->set(8, 2, true); path->set(8, 3, true); path->set(7, 3, true);
    path->set(6, 3, true); path->set(5, 3, true); path->set(4, 3, true);
    path->set(3, 3, true); path->set(2, 3, true); path->set(1, 3, true);
    path->set(1, 4, true); path->set(1, 5, true); path->set(1, 6, true);
    path->set(1, 7, true); path->set(1, 8, true); path->set(2, 8, true);
    path->set(3, 8, true); path->set(4, 8, true); path->set(4, 7, true);
    path->set(4, 6, true); path->set(4, 5, true); path->set(5, 5, true);
    path->set(6, 5, true); path->set(7, 5, true); path->set(8, 5, true);
    path->set(8, 6, true); path->set(8, 7, true); path->set(7, 7, true);
    path->set(6, 7, true); path->set(6, 8, true); path->set(6, 9, true);
    path->set(7, 9, true); path->set(8, 9, true); path->set(9, 9, true);
    
    transform::Position2 spawn_point = {
        to_x(TileCountX - 1), 
        to_z(TileCountZ - 1)
    };

    g->level = ecs.entity().set<Level>({path, spawn_point});

    for (int x = 0; x < TileCountX; x ++) {
        for (int z = 0; z < TileCountZ; z++) {
            float xc = to_x(x);
            float zc = to_z(z);

            auto t = ecs.scope<level>().entity().set<Position>({xc, 0, zc});
            if (path[0](x, z)) {
                t.is_a<prefabs::Path>();
            } else {
                t.is_a<prefabs::Tile>();

                auto e = ecs.entity().set<Position>({xc, -TileHeight / 2, zc});
                if (randf(1) > 0.9) {
                    e.child_of<level>();
                    e.is_a<prefabs::Tree>();
                } else {
                    e.child_of<turrets>();
                    if (randf(1) > 0.3) {
                        e.is_a<prefabs::Cannon>();
                    } else {
                        e.is_a<prefabs::Railgun>();
                        e.target<prefabs::Railgun::Head::Beam>().disable();
                    }
                }
            }           
        }
    }                          
}

// Init prefabs
void init_prefabs(flecs::world& ecs) {
    Game *g = ecs.get_mut<Game>();

    ecs.prefab<prefabs::Tree>();
        ecs.prefab<prefabs::Tree::Trunk>()
            .set<Position>({0, -1.0, 0})
            .set<Color>({0.25, 0.2, 0.1})
            .set<Box>({0.5, 2.0, 0.5});
        ecs.prefab<prefabs::Tree::Canopy>()
            .set<Position>({0, -3.0, 0})
            .set<Color>({0.2, 0.3, 0.15})
            .set<Box>({1.5, 2.0, 1.5})
            .override<Position>()
            .override<Box>();

    ecs.prefab<prefabs::Tile>()
        .set<Color>({0.2, 0.34, 0.15})
        .set<Specular>({0.25, 20})
        .set<Box>({TileSize, TileHeight, TileSize});

    ecs.prefab<prefabs::Path>()
        .set<Color>({0.2, 0.2, 0.2})
        .set<Specular>({0.5, 50})
        .set<Box>({TileSize + TileSpacing, PathHeight, TileSize + TileSpacing});

    ecs.prefab<prefabs::Enemy>()
        .add<Enemy>()
        .add<Health>().override<Health>()
        .set<Color>({0.1, 0.0, 0.18}).override<Color>()
        .set<Box>({EnemySize, EnemySize, EnemySize})
        .set<Specular>({4.0, 512})
        .set<SpatialQuery, Bullet>({g->center, g->size})
        .add<SpatialQueryResult, Bullet>()
        .override<SpatialQueryResult, Bullet>();

    ecs.prefab<prefabs::materials::Beam>()
        .set<Color>({0.1, 0.4, 1})
        .set<Emissive>({9.0});

    ecs.prefab<prefabs::materials::Metal>()
        .set<Color>({0.1, 0.1, 0.1})
        .set<Specular>({1.5, 128});

    ecs.prefab<prefabs::materials::CannonHead>()
        .set<Color>({0.35, 0.4, 0.3})
        .set<Specular>({0.5, 25});

    ecs.prefab<prefabs::materials::RailgunLight>()
        .set<Color>({0.1, 0.3, 1.0})
        .set<Emissive>({3.0});

    ecs.prefab<prefabs::Particle>()
        .override<ParticleLifespan>()
        .override<Color>()
        .override<Box>();

    ecs.prefab<prefabs::Bullet>().is_a<prefabs::Particle>()
        .add<Bullet>()
        .set<Color>({0, 0, 0})
        .set<Box>({BulletSize, BulletSize, BulletSize})
        .set<Particle>({
            1.0, 1.0, 1.0, BulletLifespan
        });

    ecs.prefab<prefabs::Fireball>().is_a<prefabs::Particle>()
        .set<Color>({1.0, 0.5, 0.3})
        .set<Emissive>({5.0})
        .set<Box>({FireballSize, FireballSize, FireballSize})
        .set<Particle>({
            FireballSizeDecay, 1.0, 1.0, FireballLifespan
        });

    ecs.prefab<prefabs::Smoke>().is_a<prefabs::Particle>()
        .set<Color>({0, 0, 0})
        .set<Emissive>({12.0})
        .set<Box>({SmokeSize, SmokeSize, SmokeSize})
        .set<Particle>({
            SmokeSizeDecay, SmokeColorDecay, 1.0, SmokeLifespan
        })
        .set<Velocity>({0, 0.3, 0})
        .override<Velocity>();

    ecs.prefab<prefabs::Spark>().is_a<prefabs::Particle>()
        .set<Color>({1.0, 0.5, 0.5})
        .set<Emissive>({5.0})
        .set<Box>({SparkSize, SparkSize, SparkSize})
        .set<Particle>({
            SparkSizeDecay, 1.0, SparkVelocityDecay, SparkLifespan
        });

    ecs.prefab<prefabs::Ion>()
        .is_a<prefabs::materials::Beam>()
        .is_a<prefabs::Particle>()
        .set<Box>({IonSize, IonSize, IonSize})
        .set<Particle>({
            IonDecay, 1.0, 1.0, IonLifespan
        });

    ecs.prefab<prefabs::Bolt>()
        .is_a<prefabs::materials::Beam>()
        .is_a<prefabs::Particle>()
        .set<Box>({BoltSize, BoltSize, BoltSize})
        .set<Particle>({
            BoltSizeDecay, 1.0, 1.0, BoltLifespan
        });

    ecs.prefab<prefabs::Turret>()
        .set<SpatialQuery, Enemy>({ g->center, g->size })
        .add<SpatialQueryResult, Enemy>()
        .override<SpatialQueryResult, Enemy>()
        .override<Target>()
        .override<Turret>();

        ecs.prefab<prefabs::Turret::Base>().slot()
            .set<Position>({0, 0, 0});

            ecs.prefab().is_a<prefabs::materials::Metal>()
                .child_of<prefabs::Turret::Base>()
                .set<Box>({0.6, 0.2, 0.6})
                .set<Position>({0, -0.1, 0});

            ecs.prefab().is_a<prefabs::materials::Metal>()
                .child_of<prefabs::Turret::Base>()
                .set<Box>({0.4, 0.6, 0.4})
                .set<Position>({0, -0.3, 0});

        ecs.prefab<prefabs::Turret::Head>().slot();

    ecs.prefab<prefabs::Cannon>().is_a<prefabs::Turret>()
        .set<Turret>({TurretFireInterval});

        ecs.prefab<prefabs::Cannon::Head>()
            .is_a<prefabs::materials::CannonHead>()
            .set<Box>({0.8, 0.4, 0.8})
            .set<Position>({0, -0.8, 0})
            .set<Rotation>({0, 0.0, 0});

            ecs.prefab<prefabs::Cannon::Barrel>()
                .is_a<prefabs::materials::Metal>()
                .set<Box>({0.8, 0.14, 0.14});

            ecs.prefab<prefabs::Cannon::Head::BarrelLeft>()
                .is_a<prefabs::Cannon::Barrel>()
                .set<Position>({TurretCannonLength, 0.0, -TurretCannonOffset}); 

            ecs.prefab<prefabs::Cannon::Head::BarrelRight>()
                .is_a<prefabs::Cannon::Barrel>()
                .set<Position>({TurretCannonLength, 0.0, TurretCannonOffset});                         

    ecs.prefab<prefabs::Railgun>().is_a<::prefabs::Turret>()
        .add<Railgun>()
        .set<Turret>({BeamFireInterval});    

        ecs.prefab<prefabs::Railgun::Head>()
            .set<Position>({0.0, -0.8, 0})
            .set<Rotation>({0, 0.0, 0});

            ecs.prefab().is_a<prefabs::materials::Metal>()
                .child_of<prefabs::Railgun::Head>()
                .set<Box>({0.9, 0.3, 0.16})
                .set<Position>({0.1, 0.0, -0.3});
                
            ecs.prefab().is_a<prefabs::materials::Metal>()
                .child_of<prefabs::Railgun::Head>()
                .set<Box>({0.9, 0.3, 0.16})
                .set<Position>({0.1, 0.0, 0.3});                

            ecs.prefab().is_a<prefabs::materials::RailgunLight>()
                .child_of<prefabs::Railgun::Head>()
                .set<Box>({0.8, 0.2, 0.1})
                .set<Position>({0.1, 0.0, -0.20});

            ecs.prefab().is_a<prefabs::materials::RailgunLight>()
                .child_of<prefabs::Railgun::Head>()
                .set<Box>({0.8, 0.2, 0.1})
                .set<Position>({0.1, 0.0, 0.2});

            ecs.prefab().is_a<prefabs::materials::Metal>()
                .child_of<prefabs::Railgun::Head>()
                .set<Box>({1.6, 0.5, 0.3})
                .set<Position>({0.24, 0, 0.0});  

            ecs.prefab().is_a<prefabs::materials::Metal>()
                .child_of<prefabs::Railgun::Head>()
                .set<Box>({1.0, 0.16, 0.16})
                .set<Position>({0.8, -0.1, 0.0});                  

        ecs.prefab<prefabs::Railgun::Head::Beam>().slot_of<prefabs::Railgun>()
            .is_a<prefabs::materials::Beam>()
            .set<Position>({2, 0, 0})
            .set<Box>({BeamSize, BeamSize, BeamSize}).override<Box>()
            .set<Rotation>({0,(float)GLM_PI / 2.0f, 0});
}

// Init systems
void init_systems(flecs::world& ecs) {
    ecs.scope<systems>([&](){ // Keep root scope clean

    // Spawn enemies periodically
    ecs.system<const Game>("SpawnEnemy")
        .term_at(1).singleton()
        .interval(EnemySpawnInterval)
        .iter(SpawnEnemy);

    // Move enemies
    ecs.system<Position, Direction, const Game>("MoveEnemy")
        .term_at(3).singleton()
        .term<Enemy>()
        .each(MoveEnemy);

    // Clear invalid target for turrets
    ecs.system<Target, Position>("ClearTarget")
        .each(ClearTarget);

    // Find target for turrets
    ecs.system<Turret, Target, Position>("FindTarget")
        .term<SpatialQuery, Enemy>().up()
        .term<SpatialQueryResult, Enemy>()
        .each(FindTarget);

    // Aim turret at enemies
    ecs.system<Turret, Target, Position>("AimTarget")
        .each(AimTarget);

    // Countdown until next fire
    ecs.system<Turret, Target>("FireCountdown")
        .each(FireCountdown);

    // Aim beam at target
    ecs.system<Position, Turret, Target, const Game>("BeamControl")
        .term_at(4).singleton()
        .each(BeamControl);

    // Fire bullets at enemies
    ecs.system<Turret, Target, Position, const Game>("FireAtTarget")
        .term_at(4).singleton()
        .term<Railgun>().optional()
        .each(FireAtTarget);

    // Simple particle system
    ecs.system<ParticleLifespan, const Particle, Box, Color, Velocity>
            ("ProgressParticle")
        .term_at(2).up() // shared particle properties
        .term_at(3).optional()
        .term_at(4).optional()
        .term_at(5).optional()
        .instanced()
        .iter(ProgressParticle);

    // Test for collisions with enemies
    ecs.system<Position, Health, Box>("HitTarget")
        .term<SpatialQuery, Bullet>().up()
        .term<SpatialQueryResult, Bullet>()
        .each(HitTarget);

    ecs.system<Health, Position, const Game, const graphics::Camera>
            ("DestroyEnemy")
        .term_at(3).singleton()
        .term_at(4).src("Camera")
        .term<Enemy>()
        .each(DestroyEnemy);

    ecs.system<Color, Health>("UpdateEnemyColor")
        .each(UpdateEnemyColor);

    });
}

int main(int argc, char *argv[]) {
    flecs::world ecs;

    ecs.import<flecs::components::transform>();
    ecs.import<flecs::components::graphics>();
    ecs.import<flecs::components::geometry>();
    ecs.import<flecs::components::gui>();
    ecs.import<flecs::components::physics>();
    ecs.import<flecs::components::input>();
    ecs.import<flecs::systems::transform>();
    ecs.import<flecs::systems::physics>();
    ecs.import<flecs::systems::sokol>();
    ecs.import<flecs::game>();

    init_game(ecs);
    init_ui(ecs);
    init_prefabs(ecs);
    init_level(ecs);
    init_systems(ecs);

    ecs.app()
        .target_fps(30)
        .enable_rest()
        .run();
}
