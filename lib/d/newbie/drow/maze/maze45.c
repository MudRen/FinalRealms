inherit "/std/underground";
#include "path.h"

void setup() {

    add_property("no_undead",1);

    set_light(15);

    set_short("Realm of the Drow:  The Great Maze");

    set_long("\n   Realm of the Drow:  The Great Maze.\n\n"
      "     You are in the great maze.  The room is squareish in "
      "shape and there is a pillar in each corner of the room.  "
      "The walls are made out of some plain grey stone that "
      "looks like it is almost wet.  The air is seeped with a "
      "musty smell that you can taste."
      "\n\n");

    add_item(({"pillar"}),
      "   The pillar appears to be made out of the same grey "
      "stone as the walls.  Aside from providing support, they "
      "don't seem to be of any importance.\n");

    add_clone(CHAR+"captives", random(4));
    add_clone(CHAR+"spider",3);
    add_smell("room", "It smells old.\n");
    add_taste("air", "The air tastes Musty and old.\n");

    add_exit("east",MAZE+"maze46","corridor");
    add_exit("west",MAZE+"maze44","corridor");
    add_exit("northeast",MAZE+"maze32","corridor");
    add_exit("northwest", MAZE+"maze30", "corridor");
    add_exit("southeast", MAZE+"maze60", "corridor");
    add_exit("southwest", MAZE+"maze58", "corridor");

    set_zone("maze");
}