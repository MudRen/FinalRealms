#include "path.h"
inherit "/std/outside.c";

void setup()
 {
    set_short("Clearing beneth Village");

    set_long("Clearing beneth Village.\n\n"
        "       You're standing in a large clearing beneth "
    "the elven village. During the day people usually work "
    "here, in the wide open space. The only trees around are "
    "the ones supporting the village higher up. And just south "
    "of here you see a river streaming southeastwards down the "
    "forest. There stands a small cabin in the far northern end "
    "of the clearing.     The clearing continues south and west.\n\n");
 
    set_light(80);
   
    add_item(({"tree","trees"}),"     The trees are huge, "
    "massive oaks standing steady on the ground. They give "
    "the village plenty safety just by their looks.\n");   
     
    add_item("river","     All along the river you notice "
      "some sort of trail. Perhaps it's used by the fishers...\n");   

    add_item("cabin","    It's a small wodden cabin with smoke "
    "raising from its chimney. \n");

    // add_smell(({"xxx","xxx"}),"    \n");
               
        
    add_exit("west",ROOMS+"h3.c","path");    
    add_exit("south",ROOMS+"j5.c","path");         
   }
