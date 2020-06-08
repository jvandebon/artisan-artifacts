/**
 * 3-D coordinates
 */
typedef struct {
    float x;
    float y;
    float z;
} coord3d_t;

/** Descriptor of a particle state */
typedef struct {
    coord3d_t p;
    coord3d_t v;
} particle_t;

#define EPS 100