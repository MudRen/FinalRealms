#include "path.h"
inherit "std/shop";


object baern;
object torch;

void reset()
{
    if(!baern)
    {
    baern = clone_object(CHAR +"baern.c");
    baern->move(this_object());
    }
    if(!torch)
    {
    torch = clone_object("/baseobs/misc/torch");
    torch->move(TO);
    }
}

void setup() {

    set_short("Darklor's Smithy");

    set_light(80);

    set_long("This is a small, somewhat cramped buliding that "
      "contains Darklor's Smithy.  There is a counter along "
      "the northern wall with a sign on it.  Behind the "
      "counter are a number of items that were hand crafted "
      "by one of the best blacksmiths around.  To your right "
      "sits the forge.  It does not look as if it has been "
      "in use for quite some time. "
      "\n\n");

    add_item("sign","The sign reads:\n"
      "Try /list/ to get a list of the items available.\n"
      "Try /browse/ to look at the item before you buy.\n"
      "Try /buy/ to buy the item.\n\n");
    add_item("counter","This is a simple wooden counter that runs "
      "along the northern wall.  There is a small sign sitting "
      "upon it."
      "\n\n");
    add_item("forge","The forge is cold and looks like it hasn't "
      "been in use for quite some time.  It is covered with "
      "cobwebs and dust."
      "\n\n");
    add_item("cobweb","The forge is covered with these cobwebs.  "
      "Must have been a long time since this forge was used."
      "\n\n");
    add_item("dust","There is a thick layer of dust covering the "
      "forge.  This makes the forge look old and neglected."
      "\n\n");

    add_exit("out",ROOM+"dwarf37.c","door");
    set_store_room(ROOM+"storeroom.c");
}


void init()
{
    ::init();
    add_action("do_sell","sell");
    add_action("buy_me","buy");
    add_action("do_out","out");
}
int do_sell(string str)
{
    write("Sorry, we do not purchase items here.  Try the shop across "
      "the street.  I think they might buy it.\n\n");
    return(1);
}
int buy_me(string str)
{
    if(!baern)
    {
    write("There is no one here to take your order.\n");
    return(1);
    }
}
int do_out(string str)
{
    if(this_player()->query_properties("BAERN"))
    {
    return(0);
    }
    else
    {
    return(1);
    }
}

int dest_me()
{
    if(baern)
    baern->dest_me();
    if( torch && present(torch)) torch->dest_me();
    ::dest_me();
}
int test_add(object ob, int flag)
{
    call_out("do_enter",2);
    return(1);
}
int do_enter()
{
    if(baern)
    {
    write("Baern says, 'Welcome "+this_player()->query_cap_name()+
      ", What may I do for you?'  \n\n");
    return(2);
    }
    else
    {
    return(2);
    }
}
