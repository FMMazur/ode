/*************************************************************************
 *                                                                       *
 * Open Dynamics Engine, Copyright (C) 2001,2002 Russell L. Smith.       *
 * All rights reserved.  Email: russ@q12.org   Web: www.q12.org          *
 *                                                                       *
 * This library is free software; you can redistribute it and/or         *
 * modify it under the terms of EITHER:                                  *
 *   (1) The GNU Lesser General Public License as published by the Free  *
 *       Software Foundation; either version 2.1 of the License, or (at  *
 *       your option) any later version. The text of the GNU Lesser      *
 *       General Public License is included with this library in the     *
 *       file LICENSE.TXT.                                               *
 *   (2) The BSD-style license that is included with this library in     *
 *       the file LICENSE-BSD.TXT.                                       *
 *                                                                       *
 * This library is distributed in the hope that it will be useful,       *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the files    *
 * LICENSE.TXT and LICENSE-BSD.TXT for more details.                     *
 *                                                                       *
 *************************************************************************/

// Test my Plane2D constraint.
// Uses ode-0.35 collision API.

# include       <stdio.h>
# include       <stdlib.h>
# include       <math.h>
# include       <ode/ode.h>
# include       <drawstuff/drawstuff.h>
#include "texturepath.h"


#   define drand48()  ((double) (((double) rand()) / ((double) RAND_MAX)))

# define        N_BODIES        40
# define        STAGE_SIZE      8.0  // in m

# define        TIME_STEP       0.01
# define        K_SPRING        10.0
# define        K_DAMP          10.0

//using namespace ode;

struct GlobalVars
{
    dWorld   dyn_world;
    dBody    dyn_bodies[N_BODIES];
    dReal    bodies_sides[N_BODIES][3];

    dSpaceID coll_space_id;
    dJointID plane2d_joint_ids[N_BODIES];
    dJointGroup coll_contacts;
};

static GlobalVars *g_globals_ptr = NULL;



static void     cb_start ()
/*************************/
{
    dAllocateODEDataForThread(dAllocateMaskAll);

    static float    xyz[3] = { 0.5f*STAGE_SIZE, 0.5f*STAGE_SIZE, 0.85f*STAGE_SIZE};
    static float    hpr[3] = { 90.0f, -90.0f, 0 };

    dsSetViewpoint (xyz, hpr);
}



static void     cb_near_collision (void *, dGeomID o1, dGeomID o2)
/********************************************************************/
{
    dBodyID     b1 = dGeomGetBody (o1);
    dBodyID     b2 = dGeomGetBody (o2);
    dContact    contact;


    // exit without doing anything if the two bodies are static
    if (b1 == 0 && b2 == 0)
        return;

    // exit without doing anything if the two bodies are connected by a joint
    if (b1 && b2 && dAreConnected (b1, b2))
    {
        /* MTRAP; */
        return;
    }

    contact.surface.mode = 0;
    contact.surface.mu = 0; // frictionless

    if (dCollide (o1, o2, 1, &contact.geom, sizeof (dContactGeom)))
    {
        dJointID c = dJointCreateContact (g_globals_ptr->dyn_world.id(),
                        g_globals_ptr->coll_contacts.id (), &contact);
        dJointAttach (c, b1, b2);
    }
}


static void     track_to_pos (dBody &body, dJointID joint_id,
                              dReal target_x, dReal target_y)
/************************************************************************/
{
    dReal  curr_x = body.getPosition()[0];
    dReal  curr_y = body.getPosition()[1];

    dJointSetPlane2DXParam (joint_id, dParamVel, 1 * (target_x - curr_x));
    dJointSetPlane2DYParam (joint_id, dParamVel, 1 * (target_y - curr_y));
}



static void     cb_sim_step (int pause)
/*************************************/
{
    if (! pause)
    {
        static dReal angle = 0;

        angle += REAL( 0.01 );

        track_to_pos (g_globals_ptr->dyn_bodies[0], g_globals_ptr->plane2d_joint_ids[0],
            dReal( STAGE_SIZE/2 + STAGE_SIZE/2.0 * cos (angle) ),
            dReal( STAGE_SIZE/2 + STAGE_SIZE/2.0 * sin (angle) ));

        /* double   f0 = 0.001; */
        /* for (int b = 0; b < N_BODIES; b ++) */
        /* { */
            /* double   p = 1 + b / (double) N_BODIES; */
            /* double   q = 2 - b / (double) N_BODIES; */
            /* g_globals_ptr->dyn_bodies[b].addForce (f0 * cos (p*angle), f0 * sin (q*angle), 0); */
        /* } */
        /* g_globals_ptr->dyn_bodies[0].addTorque (0, 0, 0.1); */

        const int n = 10;
        for (int i = 0; i < n; i ++)
        {
            dSpaceCollide (g_globals_ptr->coll_space_id, 0, &cb_near_collision);
            g_globals_ptr->dyn_world.step (dReal(TIME_STEP/n));
            g_globals_ptr->coll_contacts.empty ();
        }
    }

# if 1  /* [ */
    {
        // @@@ hack Plane2D constraint error reduction here:
        for (int b = 0; b < N_BODIES; b ++)
        {
            const dReal     *rot = dBodyGetAngularVel (g_globals_ptr->dyn_bodies[b].id());
            const dReal     *quat_ptr;
            dReal           quat[4],
                            quat_len;


            quat_ptr = dBodyGetQuaternion (g_globals_ptr->dyn_bodies[b].id());
            quat[0] = quat_ptr[0];
            quat[1] = 0;
            quat[2] = 0;
            quat[3] = quat_ptr[3];
            quat_len = sqrt (quat[0] * quat[0] + quat[3] * quat[3]);
            quat[0] /= quat_len;
            quat[3] /= quat_len;
            dBodySetQuaternion (g_globals_ptr->dyn_bodies[b].id(), quat);
            dBodySetAngularVel (g_globals_ptr->dyn_bodies[b].id(), 0, 0, rot[2]);
        }
    }
# endif  /* ] */


# if 0  /* [ */
    {
        // @@@ friction
        for (int b = 0; b < N_BODIES; b ++)
        {
            const dReal *vel = dBodyGetLinearVel (g_globals_ptr->dyn_bodies[b].id()),
                        *rot = dBodyGetAngularVel (g_globals_ptr->dyn_bodies[b].id());
            dReal       s = 1.00;
            dReal       t = 0.99;

            dBodySetLinearVel (g_globals_ptr->dyn_bodies[b].id(), s*vel[0],s*vel[1],s*vel[2]);
            dBodySetAngularVel (g_globals_ptr->dyn_bodies[b].id(),t*rot[0],t*rot[1],t*rot[2]);
        }
    }
# endif  /* ] */


    {
        // ode  drawstuff

        dsSetTexture (DS_WOOD);
        for (int b = 0; b < N_BODIES; b ++)
        {
            if (b == 0)
            dsSetColor (1.0, 0.5, 1.0);
            else
            dsSetColor (0, 0.5, 1.0);
#ifdef dDOUBLE
            dsDrawBoxD (g_globals_ptr->dyn_bodies[b].getPosition(), g_globals_ptr->dyn_bodies[b].getRotation(), g_globals_ptr->bodies_sides[b]);
#else
            dsDrawBox (g_globals_ptr->dyn_bodies[b].getPosition(), g_globals_ptr->dyn_bodies[b].getRotation(), g_globals_ptr->bodies_sides[b]);
#endif
        }
    }
}



extern int      main
/******************/
(
    int         argc,
    char        **argv
)
{
    int         b;
    dsFunctions drawstuff_functions;


    dInitODE2(0);

    g_globals_ptr = new GlobalVars();

    // dynamic world

    dReal  cf_mixing;// = 1 / TIME_STEP * K_SPRING + K_DAMP;
    dReal  err_reduct;// = TIME_STEP * K_SPRING * cf_mixing;
    err_reduct = REAL( 0.5 );
    cf_mixing = REAL( 0.001 );
    dWorldSetERP (g_globals_ptr->dyn_world.id (), err_reduct);
    dWorldSetCFM (g_globals_ptr->dyn_world.id (), cf_mixing);
    g_globals_ptr->dyn_world.setGravity (0, 0.0, -1.0);

    g_globals_ptr->coll_space_id = dSimpleSpaceCreate (0);

    // dynamic bodies
    for (b = 0; b < N_BODIES; b ++)
    {
        int     l = (int) (1 + sqrt ((double) N_BODIES));
        dReal  x = dReal( (0.5 + (b / l)) / l * STAGE_SIZE );
        dReal  y = dReal( (0.5 + (b % l)) / l * STAGE_SIZE );
        dReal  z = REAL( 1.0 ) + REAL( 0.1 ) * (dReal)drand48();

        g_globals_ptr->bodies_sides[b][0] = dReal( 5 * (0.2 + 0.7*drand48()) / sqrt((double)N_BODIES) );
        g_globals_ptr->bodies_sides[b][1] = dReal( 5 * (0.2 + 0.7*drand48()) / sqrt((double)N_BODIES) );
        g_globals_ptr->bodies_sides[b][2] = z;

        g_globals_ptr->dyn_bodies[b].create (g_globals_ptr->dyn_world);
        g_globals_ptr->dyn_bodies[b].setPosition (x, y, z/2);
        g_globals_ptr->dyn_bodies[b].setData ((void*) (dsizeint)b);
        dBodySetLinearVel (g_globals_ptr->dyn_bodies[b].id (),
            dReal( 3 * (drand48 () - 0.5) ), 
			dReal( 3 * (drand48 () - 0.5) ), 0);

        dMass m;
        m.setBox (1, g_globals_ptr->bodies_sides[b][0],g_globals_ptr->bodies_sides[b][1],g_globals_ptr->bodies_sides[b][2]);
        m.adjust (REAL(0.1) * g_globals_ptr->bodies_sides[b][0] * g_globals_ptr->bodies_sides[b][1]);
        g_globals_ptr->dyn_bodies[b].setMass (&m);

        g_globals_ptr->plane2d_joint_ids[b] = dJointCreatePlane2D (g_globals_ptr->dyn_world.id (), 0);
        dJointAttach (g_globals_ptr->plane2d_joint_ids[b], g_globals_ptr->dyn_bodies[b].id (), 0);
    }

    dJointSetPlane2DXParam (g_globals_ptr->plane2d_joint_ids[0], dParamFMax, 10);
    dJointSetPlane2DYParam (g_globals_ptr->plane2d_joint_ids[0], dParamFMax, 10);


    // collision geoms and joints
    dCreatePlane (g_globals_ptr->coll_space_id,  1, 0, 0, 0);
    dCreatePlane (g_globals_ptr->coll_space_id, -1, 0, 0, -STAGE_SIZE);
    dCreatePlane (g_globals_ptr->coll_space_id,  0,  1, 0, 0);
    dCreatePlane (g_globals_ptr->coll_space_id,  0, -1, 0, -STAGE_SIZE);

    for (b = 0; b < N_BODIES; b ++)
    {
        dGeomID coll_box_id;
        coll_box_id = dCreateBox (g_globals_ptr->coll_space_id,
            g_globals_ptr->bodies_sides[b][0], g_globals_ptr->bodies_sides[b][1], g_globals_ptr->bodies_sides[b][2]);
        dGeomSetBody (coll_box_id, g_globals_ptr->dyn_bodies[b].id ());
    }

    g_globals_ptr->coll_contacts.create ();

    {
        // simulation loop (by drawstuff lib)
        drawstuff_functions.version = DS_VERSION;
        drawstuff_functions.start = &cb_start;
        drawstuff_functions.step = &cb_sim_step;
        drawstuff_functions.command = 0;
        drawstuff_functions.stop = 0;
        drawstuff_functions.path_to_textures = DRAWSTUFF_TEXTURE_PATH;

        dsSimulationLoop (argc, argv, DS_SIMULATION_DEFAULT_WIDTH, DS_SIMULATION_DEFAULT_HEIGHT, &drawstuff_functions);
    }

    delete g_globals_ptr;
    g_globals_ptr = NULL;

	dCloseODE();
    return 0;
}
