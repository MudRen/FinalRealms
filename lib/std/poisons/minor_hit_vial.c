/*** Poison Vial ***/
/*** Minor hit Poison type. ***/
/*** by Wonderflug **/

inherit "/std/item";
#define MYPOISON "/std/poisons/shadows/minor_hit_sh.c"
#define XLEVEL 4
#define MAXUSES 3

void setup()
{
  set_name("vial of poison");
  add_alias("vial");
  add_alias("red");
  set_short("Vial of Red Liquid");
  set_long("\nVial of Red Liquid\n\n"
    "     This is a small vial of poison, made of some extremely hard "
    "crystalline substance; lucky it is too, for you shudder to think "
    "how quickly you'd be dead if it broke and you spilled any of the "
    "deadly poison inside upon you.  The cap has a dropper attached to "
    "it so all you have to do is 'annoint' a weapon with the poison.  "
    "Even so, only professionals should attempt this and even they "
    "should be careful to have both hands free when doing it.  Also, "
    "agitation makes even the most careful hand careless.\n");
  set_read_mess("\nStirge Blood.\n\nGathered deep underground, this poison "
    "is fairly readily available from the huge flocks of stirges that "
    "can be found.  It's known to cause acute pain in a wound, but "
    "it dries away almost instantly.\n"
    "Strength: Minor\n\n");
  set_main_plural("Vials of Liquid");
  set_weight(10);
  set_value(5000);
  add_property("num_annoints",MAXUSES);
}

void init()
{
  ::init();
  add_action("do_annoint", "annoint");
}

int check_guild()
{
  switch( this_player()->query_guild_name() )
  {
     case "assassin":
    break;
     default:
    return 0;
  }
  if ( this_player()->query_level() < XLEVEL )
    return 0;
  return 1;
}

int do_annoint( string str )
{
  object* held_obs;
  object* obs;
  object weap;
  object poison;
  int i;

/* Check to make sure they're an assassin (or valid guild anyway) */
  if ( !check_guild() )
  {
    tell_object(this_player(), "You really have no idea what you're "
      "doing with the poison. Maybe you should \nleave that kind of "
      "thing to more experienced professionals.\n\n");
/* They're not, so if they fail a dex check they nick themselves. */
    if ( (int)this_player()->query_dex() < random(20) )
    {
      tell_object(this_player(), "Oops!  You spilled some on you.\n");
      tell_room(environment(this_player()), this_player()->query_cap_name()+
        " spills some poison on "+this_player()->query_objective()+
        "self.\n", this_player());
      MYPOISON->do_poison_effect(this_player(), this_player());
    }
    return 1;
  }

  if ( !str )
  {
    tell_object(this_player(),"Syntax: annoint <weapon>\n");
    return 1;
  }
 
  obs = find_match( str, this_player() );

  if ( !sizeof(obs) )
  {
    tell_object(this_player(), "You can't annoint something you don't "
      "have!\n");
    return 1;
  }

  weap = obs[0];
/* Test if we can add a poison to the weapon.  In a poison shadow, this'll
 * return 0 and probably do the effect of the poison to the annointee.  
 * Otherwise it won't even be there and evaluate to zero anyway.  Only on
 * the poison blades will it return 1.
 */
  if ( !(weap->test_add_poison()) )
  {
    tell_object(this_player(), "You cannot annoint "+weap->query_name()+
      ".\n");
    return 1;
  }

/* See if the player's holding anything.  If so we won't let him put the
 * poison on, and they have to make a harsh dex check to avoid poisoning
 * themselves.  Just an annoyance for them cause I'm evil.
 */
  held_obs = this_player()->query_held_ob();
  if (held_obs[0] != 0 || held_obs[1] != 0)
  {
    tell_object(this_player(), "You don't have 4 hands!  Maybe try "
      "putting something down before \ntrying to poison your weapons.\n");
    if ( (int)this_player()->query_dex() < random(30) )
    {
      tell_object(this_player(), "Oops!  Now you've gone and spilled "
        "some on you!.\n");
      tell_room(environment(this_player()), this_player()->query_cap_name()+
        " spills some poison on "+this_player()->query_objective()+
        "self.\n", this_player());
      MYPOISON->do_poison_effect(this_player(), this_player());
    }
    return 1;
  }

/* Make sure they're not already in combat.  If they are, they automatically
 * hit themselves.
 */
  if ( sizeof(this_player()->query_attacker_list()) )
  {
    tell_object(this_player(), "Being so agitated as you are, "
      "you spill the poison all \nover yourself.\n");
    tell_room(this_player(), this_player()->query_cap_name()+
      " fumbles a Vial of Red Liquid and spills some on "+
      this_player()->query_objective()+"self.\n", this_player());
    MYPOISON->do_poison_effect(this_player(),this_player());
    return 1;
  }

/* Now everything looks okay and we can put the shadow on the weapon.
 */
  poison = clone_object(MYPOISON);
  poison->setup_shadow(weap);
  tell_room(environment(this_player()), this_player()->query_cap_name()+ 
    " annoints a "+weap->query_short()+" with a Vial of Red Liquid.\n",
    this_player());
  tell_object(this_player(), "You annoint your "+weap->query_short()+
    " with poison from your vial.\n");

/* Now decrement the number of uses left and if it's none, dest the object
 * and return.
 */
  if ( (i = (int)this_object()->query_property("num_annoints")) <= 0 )
  {
    tell_object(this_player(),"You've used up all the poison in the "
      "vial.\n");
    this_object()->dest_me();
  }
  else this_object()->add_property("num_annoints", i-1);
  return 1;
}
