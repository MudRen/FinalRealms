//Ljen nov -96

inherit "/std/room";
#include "path.h"

void setup(){
   set_short("Castle Of The Green Sun: Corridor");
   set_long("You are standing in the corridor of the castle's first floor,"
            " which leads east and west. To the west are the living rooms"
        " and to the east are the sleeping quarters."
            " The walls and ceiling are bare but the torches.  On the floor"
            " there is a carpet all along with some strange drawings in"
            " it.\n");

   add_item("torch", "A common torch with a wooden haft and a cloth head "
                     "soaked in oil. It gives a soft light to the room.\n");   
 
   add_item("carpet", "A red carpet with golden drawings.  The drawings"
            "seem to be a very old language where you can understand some"
            "words: '.... Mirror ..exit.... treasure....  pull.'\n");

  set_light(50);

   add_exit("east", CAST+"d4.c",  "door" );
   add_exit("west", CAST+"b4.c", "door");
}



