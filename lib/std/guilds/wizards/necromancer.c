inherit "/std/guilds/mage";

void setup()
{
  set_name("necromancer");
  set_short("necromancer");
  set_long(" \n");
 /** insert description of specialist above ***/
  reset_get();
}

int query_legal_race(string race)
{
    switch(race)
    {
     case "human":
      return 1;

     default:
      return 0;
    }
}

mixed query_legal_spheres()
{
   return ({"abjuration","alteration","lesserdivination",
       "greaterdivination", "necromancy",
       "invocation","conjuration"});
}
