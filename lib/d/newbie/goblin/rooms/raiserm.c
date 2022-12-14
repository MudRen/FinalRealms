/* A temple for raising..
 * Needs money to work...
 * Inherits /std/raiseroom.c that uses the bank-handler to handle accounts.
 * Baldrick.
 */

#include "path.h"
inherit "/std/raiseroom";
#include "path.h";

object helper;

void setup()
  {
   set_helper_name("shaman");
set_short("Realm of the Goblin:  Raiseroom");
set_save_file("collect");
set_long("\nRealm of the Goblin:  Raiseroom\n\n"
"     This is the room you come to when you are dead and wish to get raised. "
    "It is a quiet, peaceful room, and in contrast to the rest of the village, "
"it has been well-maintained. The walls and floor have been freshly "
   "scrubbed, and there is a pleasant odor in the air.\n"
         "If you want to get raised, you might want to look at the"
     " sign first. \n\n");
add_property("no_undead",1);
  set_percentage(100);
  set_light(60);

    add_smell("air","There is a pleasant smell of roses in the air.\n");
add_item("shrine", "A small altar to Tymora sits in the corner.  There"
    " are several lit candles around the shrine, but it"
    " really doesn't attract your attention when you first"
    " enter the room. \n");
add_item(({"cot", "cots"}), "Uncomfortable looking wooden tables where the"
    " bodies are placed for examination and treatment. \n");
add_item(({"healer", "healers"}), "Stern looking individuals who spend their"
    " days raising people from the dead.  They move about the"
    " room, their movements fluid and purposeful.  They are busy"
    " people, and don't look like they smile often. \n");
  add_sign("/---------------------------------------------\\\n" 
           "|                                             |\n" 
           "|            Raiseroom                        |\n"
           "|                                             |\n" 
           "|     Type raise <ghost> to raise someone.    |\n" 
           "|_____________________________________________|\n" 
             "\n\n");


   add_exit("west",ROOM+"guildrm3.c","path");
}

void reset()
  {
  if (!helper)
    {
   helper = clone_object("/baseobs/monsters/healer");
    helper->move(this_object());
    }

  helper->set_name("Goblin Shaman");
  helper->set_short("Shaman");
helper->add_alias("shaman");
helper->set_long("The goblin male that stands before is a shaman to the "
  "goblin god Maglubiyet. He attends to the wounds of the many "
  "goblin adventurers. It is said that he can even bring some back "
   "from the dead.\n\n");
} /* void reset */ 

void dest_me()
  {
  if (helper)
    helper->dest_me();
} /* void dest me */
