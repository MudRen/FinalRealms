#include "path.h"

inherit ROOMS+"gen.c";
void setup(){my_setup();
    set_short("Highlo : Forest Hill  \n");
    set_light(LIGHT);
    add_property("no_undead",1);
    set_long("Forest Hill.  \n\n"
      "\n\n");


    add_item(({"tree","trees","forest"}),TREE);
    add_sound(({"birds","wildlife","bird","songs"}),BIRDS);
    add_feel(({"ground","dirt","floor"}),GROUND);
    add_exit("northwest",ROOMS+"ken4","path");
    add_exit("south",ROOMS+"ken19","path");
    add_exit("east",ROOMS+"ken18","path");
    add_exit("southeast",ROOMS+"ken20");
}


