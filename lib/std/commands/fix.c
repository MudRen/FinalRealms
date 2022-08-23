
#include "money.h"
int fix(string str,object fixer)
{
  object *obs, *too_poor, *ok, *fail;
  int w_skill, a_skill, low_c, max_c, high_c, total_cost, per, i, diff, val;
  if(!fixer) fixer = this_player();
  if (!str)
  {
    notify_fail("Syntax: fix <armor(s)/weapon(s)>\n"
        "PS you need to be in a smithy.\n");
    return 0;
  }
  if (!environment(fixer)->query_property("smithy"))
  {
    notify_fail("You are not in a smithy so you cannot fix anything.\n");
    return 0;
  }
  obs = find_match(str, fixer); /* can only fix things your
                     * holding */
  obs = filter_array(obs, "check_arm_or_weap", this_object());
  if (!sizeof(obs))
  {
    notify_fail("You need to actually fix something.\n");
    return 0;
  }
   a_skill = (int)this_player()->query_level() * 10;
    if(a_skill > 90) a_skill = 90;
  ok = too_poor = fail = ({ });
  val = (int)this_player()->query_value();
  for (i=0;i<sizeof(obs);i++)
  {
    low_c = (int)obs[i]->query_lowest_cond();
    max_c = (int)obs[i]->query_max_cond();
    diff = (int)obs[i]->query_cond();
    val = obs[i]->query_value();
    diff = ((diff - low_c)*100)/max_c;
    if (diff < 0) diff = 0;
    if (!diff)
    {
      fail += ({ obs[i] });
      continue;
    }
    val = (diff * val)/100;
    val = (val * (100 - a_skill))/100;
    if (val < (diff*10)/100 + total_cost) {
      too_poor += ({ obs[i] });
      continue;
    }
    ok += ({ obs[i] });
    obs[i]->adjust_cond(1);
    total_cost += val;
  }
  if (total_cost > 1)
    fixer->pay_money((mixed*)MONEY_HAND->create_money_array(total_cost));
  if (sizeof(fail))
    tell_object(fixer,"You failed to fix "+query_multiple_short(fail)+".\n");
  if (sizeof(too_poor))
    tell_object(fixer,"You are too poor to afford the materials to fix "+
      query_multiple_short(too_poor)+".\n");
  if (sizeof(ok))
  {
    tell_object(fixer,"You fix "+query_multiple_short(ok)+" and it costs you "+
      MONEY_HAND->money_value_string(total_cost)+".\n");
    tell_room(environment(fixer),fixer->query_cap_name()+" fixes up "+
    query_multiple_short(ok)+".\n");
  }
  return 1;
}
int check_arm_or_weap(object ob)
{
  return (int)ob->query_weapon() || (int)ob->query_armour();
}
string help()
{
  return
"Syntax: fix <objects>\n\n"
"This command allows you to fix up damaged weapons and armors.  Using "
"weapons and armors in combat deteriorates their condition.  Using this "
"command you can fix up the armor or weapon's condition so that it gives "
"you more protection or does more damage again.  It costs you 10 copper "
"pieces per point of condition you fix and you need to do it in a "
"smithy.  You can only fix a weapon or armor so far above its minimun "
"condition it has ever achieved.\n\n"
"Examples\n"
"> fix sword\n"
"You fix up a long sword and it costs you 6 silver pieces.\n";
}

