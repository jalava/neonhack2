/* NetHack 5.0	stairs.h	$NHDT-Date: 1685863327 2023/06/04 07:22:07 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.47 $ */
/* Copyright (c) 2024 by Pasi Kallinen */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef STAIRS_H
#define STAIRS_H

typedef struct stairway { /* basic stairway identifier */
    coordxy sx, sy;         /* x / y location of the stair */
    d_level tolev;        /* where does it go */
    boolean up;           /* up or down? */
    boolean isladder;     /* ladder or stairway? */
    boolean u_traversed;  /* hero has traversed this stair */
    xint8 kind;           /* NeonHack 2: STWAY_xxx */
    struct stairway *next;
} stairway;

/* stairway kinds */
#define STWAY_NORMAL 0
#define STWAY_METRO 1 /* a metro station; goes wherever the map says */

#endif /* STAIRS_H */

