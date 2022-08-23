inherit "/std/room";
#include "path.h"

void setup() {
set_short("The Great Maze");
set_light(15); add_item("piller","The pillar appears to be made out of the same grey stone as the walls, and doesn't appear to be of any importance.\n");
set_long("You are in the great maze.  The room is squareish in shape and "
     "there is a pillar in each corner of the room.  The walls are made "
     "out of some plain grey stone that looks like it is almost wet. "
     "The air is seeped with a musty smell that you can taste."
      "\n");
add_item("hole","The hole in the floor looks like the only way out "
     "of the room.\n");
add_smell("room", "It smells old.\n");
add_taste("air", "The air tastes Musty and old.");

add_exit("hole",MAZE+"maze54","hole");

 add_clone(CHAR+"capelf3",1);
add_property("no_undead",1);   }  /* setup */
