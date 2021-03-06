#include "physics.h"
#include <math.h>
#include <cilk/cilk.h>
#include <cilk/cilk_api.h>

// Formula: a_i = G * sum[(m_j * r_ij) / (r_ij^2 + EPS^2)^(3/2)]
// G is multiplied at the update step.
void physics_calc_force(node* n, particle* p) {
    ALIGNED real d[3];
    d[:] = n->mass_center[:] - p->position[:]; //dx, dy, dz;
    ALIGNED real dist = __sec_reduce_add(d[:] * d[:]); //dx * dx + dy * dy + dz * dz;
    if (dist < EPS2) {
        dist += EPS2;
    }
    
    if (n->size == 1 || ((n->radius / sqrt(dist)) < THETA)) { // External node or close Interlan node
        ALIGNED real force = n->mass_total / sqrt(dist * dist * dist);
        p->force[:] += d[:] * force;
    } else { // Otherwise visit all internal node children.
        if (n->next[:]) {
            physics_calc_force(n->next[:], p);
        }
    }
}

void physics_apply_force(particle* p, real* max_lim, real* min_lim) {
    
    // Update the position.
    p->force[:] *= G;
    
    p->position[:] += (p->velocity[:] + 0.5 * p->force[:] * TIME_STEP) * TIME_STEP;
    
    // Update the velocity.
    p->velocity[:] += p->force[:] * TIME_STEP;
    
    // Reset the force.
    p->force[:] = 0.0;
    
    // Bounce.
    for (char i = 0; i < 3 ;i++) {
        if (p->position[i] >= max_lim[i]) {
            p->position[i] = max_lim[i] - (p->position[i] - max_lim[i]) * 0.0001;
            p->velocity[i] *= -BOUNCE_FACTOR;
        } else if (p->position[i] <= min_lim[i]) {
            p->position[i] = min_lim[i] - (p->position[i] - min_lim[i]) * 0.0001;
            p->velocity[i] *= -BOUNCE_FACTOR;
        }
    }
}
