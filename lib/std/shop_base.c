inherit "/std/room";
/* Reworked by Hamlet July/August 1995. */
/* New features:
        1)  DOCUMENTATION!  
        2)  A more sensical scaled_value()  (makes more sense to me at least)
    3)  Ability to force-set the price the shop will offer for an item;
        set_resale_value(), adjust_resale_value(), query_resale_value(),
        prevent_resale(), and allow_resale()  all added to 
        /std/basic/misc.c (so, /std/object).
    4)  Stolen items are worth less now.  % modifier and 
        set_stolen_modifier() to alter that modifier.  Also
        set_stolen_modifier(), query_stolen_modifier(), and
        no_sell_if_stolen() in /std/basic/misc.c
    5)  A shop can be sell-only.  The commands "value" and "sell"
        are turned off for that.  Put sell_only() in setup();
        6)  You can specify a special function in your shop to be run
            ever time something is sold or bought.  call is
            set_sell_func("func") and set_buy_func("func");
            func should look like:  void func(object *stuff);
            Yes, this means that stuff is an ARRAY of items being bought
            or sold.
    
*/

// Edited 11 SEP 94 by Timion and Taniwha to fix the sell item bug.

/*
 * Original made who knows when.
 * Modified by bil to make the list a lot nicer.
 * Modified by Pinkfish to give shops types and make them send out
 * reps to sell/buy things from other shops.
 */

/* I am not sure whose code is whose any more.  This *is* an extensive
   rewrite, and a lot of it is now mine.  Credits stay, though, since
   I don't know which is which. - Hamlet
*/
/* Note:  rewrite is not done.  A lot of the code is original still.
          Don't expect that to be true for long (muhahaha)
*/

#include "money.h"
#include "move_failures.h"
#include "shop.h"
#include "armoury.h"

mixed our_storeroom;
mixed buy_mess, sell_mess, list_mess, value_mess, browse_mess,
      open_cond, *other_shops;
int amount_sold,
    amount_bought;
 string shop_type;
int stolen_modifier;  /* This will be a percent value */
int only_sell = 0;
string sell_func, buy_func;

 string query_name(object ob);
 string query_short(object ob);
int check_inv(object ob, string str);
string shop_list(mixed arr, int detail);
void do_buy(object *ob, int cost, object pl);
string shop_parse(string str, mixed ob, object client, string money,
                              string extra);
void do_parse(mixed arr, mixed ob, object client, string money,
                              string extra);

void create() {
  buy_mess = ({ "You buy $ob$ for $money$.\n",
                "$client$ buys $ob$.\n" });
  sell_mess = ({ "You sell $ob$ for $money$.\n",
                 "$client$ sells $ob$.\n" });
  list_mess = "$extra$";
  value_mess = "The $ob$ is valued at $money$.\n";
  browse_mess = "The $ob$ costs $money$, it looks like:\n$extra$";
  open_cond = 1;
  other_shops = ({ });
   shop_type = "general";
  stolen_modifier = 33;  /* 33% of normal value */
  ::create();
}

/* These make spiffy messages for various actions. */
void set_sell_message(mixed str) { sell_mess = str; }
void set_buy_message(mixed str) { buy_mess = str; }
void set_value_message(mixed str) { value_mess = str; }
void set_browse_message(mixed str) { browse_mess = str; }
void set_list_message(mixed str) { list_mess = str; }

void set_open_condition(mixed str) { open_cond = str; }

mixed query_sell_mess() { return sell_mess; }
mixed query_list_mess() { return list_mess; }
mixed query_value_mess() { return value_mess; }
mixed query_buy_mess() { return buy_mess; }
mixed query_browse_mess() { return browse_mess; }

/* These initialize sell_func and buy_func, which are special functions
   that can be set to run when something is bought or sold.
*/
void set_sell_func(string str) {  sell_func = str; }
void set_buy_func(string str) { buy_func = str; }

mixed query_open_condition() { return open_cond; }

/* This is whether the shop is open or not.  open_cond can be
   an int (true/false). Or a string, in which case it 'points'
   to a function in the current object to run.  OR, it can be
   an array.  ({ "object/name", "function_name" }) where it runs
   "object/name"->function_name() to determine whether the shop is
   open.
*/
int test_open() {
  if (stringp(open_cond))
    return (int)call_other(this_object(), open_cond);
  if (intp(open_cond))
     return open_cond;
  return (int)call_other(open_cond[0], open_cond[1]);
}

int sell_only() {  only_sell = 1; return 1; }

void init() {
  ::init();
  add_action("sell", "sell");
  add_action("buy", "buy");
  add_action("list", "list");
  add_action("browse", "browse");
  add_action("value", "value");
}

/* This determines how much the shopkeeper will offer for an item, according
   to its value.  It only gets called if set_no_resell() has not been called
   and resale_value has not been hand-set to something.
*/
int scaled_value(int n)
{
   int *prates;
   int i, tot;

   tot = 0;
   prates = PAY_RATES;
   for(i=0;( (i<sizeof(prates)) && (n > prates[i]) );i+=2);
   
   if(i>0)
     i-=2;/* prates[i] is now the rate-increment directly below the value. */

   tot = (n - prates[i]) * ((prates[i+2] / prates[i+3]) - 
           (prates[i] / prates[i+1]));
   tot /= (prates[i+2] - prates[i]);
   tot += (prates[i] / prates[i+1]);
       
   /* For those curious, we just defined a line segment.  Basically, we
      used  y - y1 = m(x - x1).  And found the y value for x = n.
      Read shop.h for better explanation.
   */
       
   return tot;
}

int set_stolen_modifier(int amt) { 
  if(amt > 100)
    amt = 100;
  if(amt < 0)
    amt = 0;
  
  stolen_modifier = amt;
  
  return amt;
}

/* This checks the player to make sure they have the item. 
   I *think* this is a paranoia check, but I'll leave it.
*/
int check_inv(object thing, string str)
{
  int i;
  object *inv = find_match(str,this_player());
  
  for(i =0; i < sizeof(inv); i++) {
    if(inv[i] == thing) return(1);
  }
  
  return(0);
}

int sell(string str) {
  object *obs, *selling, *cannot;
  mixed *m_array;
  int i, j, no, amt, value, total_amt;
   string s;

  if (!test_open())
    return 0;
  
  if(only_sell) {
    tell_object(this_player(),"This shop does not buy merchandise.\n");
    return 1;
  }
  
  if (!str || str == "") {
    notify_fail("Usage: sell <objects>\n");
    return 0;
  }
  obs = find_match(str, this_player());
  if (!sizeof(obs)) {
    notify_fail("Nothing to sell.\n");
    return 0;
  }
  if(sizeof(obs) > MAX_OBS) {
    write("The shopkeeper can't cope with all those objects.\n");
    obs = obs[0..MAX_OBS - 1];
  }
  selling = cannot = ({ });
  for (i=0;i<sizeof(obs);i++) {
    if( (obs[i]->query_value() > 0) && !obs[i]->do_not_sell() &&
        (obs[i]->query_resale_value() != -1) && 
        ((obs[i]->query_stolen_modifier() != -1) || 
          !obs[i]->query_property("stolen")) && !obs[i]->query_in_use()) {
            /* O.K. this SHOULD check to see if the item IS in the inventory 
               before we move it */
       if(check_inv(obs[i],str)) {
        if (obs[i]->move(our_storeroom)) {
          if (obs[i]->short())
            cannot += ({ obs[i] });
          continue;
        }
      /* the call other (buried) in the below is so that we can update 
         /std/shop and have all the shops prices fall in line consistently */
      amt = obs[i]->query_resale_value(); 
      if(!amt) {
        amt = (int)"/std/shop"->scaled_value((int)obs[i]->query_value());
        if(amt > MAX_AMOUNT)
          amt = MAX_AMOUNT;
      }
      
      /* Let's not let something sell back for more than it was bought for */
      if(amt > obs[i]->query_value())
        amt = obs[i]->query_value();

      /* "hot" goods lose value!  */      
      if(obs[i]->query_property("stolen")) {
        if(obs[i]->query_stolen_modifier() == 0)
          amt = amt * stolen_modifier / 100;
        else
          amt = amt * obs[i]->query_stolen_modifier() / 100;
      }
        
      total_amt += amt;
      selling += ({ obs[i] });
      obs[i]->being_sold();
    } else if (obs[i]->short())
        cannot += ({ obs[i] });
  /* end of test if exists, not that this DOESN'T do the cannot that it should
*/
          }
    else if (obs[i]->short())
      cannot += ({ obs[i] });
  }
  if (!sizeof(selling)) {
     if (sizeof(cannot))
      notify_fail("You cannot sell "+query_multiple_short(cannot)+", maybe you are holding or wearing it, or just don't have one.\n");
    else
      notify_fail("Nothing to sell.\n");
    return 0;
  }
  amount_sold += total_amt;
  m_array = (mixed *)MONEY_HAND->create_money_array(total_amt);
  this_player()->adjust_money(m_array);
  if (sizeof(cannot))
    write("You cannot sell "+query_multiple_short(cannot)+", maybe you are wearing or holding it, or just don't have one.\n");
  do_parse(sell_mess, selling, this_player(),
                (string)MONEY_HAND->money_string(m_array), "");
  if(sell_func)  call_other(this_object(),sell_func,selling);
  return 1;
}

int buy(string str) {
  int i, amt, ob_amt, total_cost;
  object *obs, *to_buy, *cannot, *too_much;
  string s;

  if (!test_open())
     return 0;

  if (!str || str == "") {
    notify_fail("Usage: buy <objects>\n");
    return 0;
  }
  obs = find_match(str, our_storeroom);
  if (!sizeof(obs)) {
    notify_fail("Cannot find "+str+".\n");
    return 0;
  }
  if(sizeof(obs) > MAX_OBS) {
    write("The shopkeeper can't cope with all those objects.\n");
    obs = obs[0..MAX_OBS-1];
  }
  to_buy = too_much = cannot = ({ });
  amt = (int)this_player()->query_value();
  while (i<sizeof(obs)) {
    if ((ob_amt = (int)obs[i]->query_value()) > amt) {
      if (obs[i]->short())
        too_much += ({ obs[i] });
      obs = delete(obs, i, 1);
       continue;
    }
    if (obs[i]->move(this_player())) {
      if (obs[i]->short())
        cannot += ({ obs[i] });
       i++;
      continue;
    }
    obs[i]->move(our_storeroom);
    amt -= ob_amt;
    total_cost += ob_amt;
    to_buy += ({ obs[i] });
    i++;
  }
  amount_bought += total_cost;
  s = "";
  if (sizeof(cannot))
    s += "You cannot pick up "+query_multiple_short(cannot)+".\n";
  if (sizeof(too_much))
    s += capitalize(query_multiple_short(too_much))+" costs too much.\n";
  if(!sizeof(to_buy)) {
    if(s != "")
      notify_fail(s);
    else
      notify_fail("Nothing to buy.\n");
    return 0;
   } else {
    write(s);
  }
  do_buy(to_buy, total_cost, this_player());
  if(buy_func)  call_other(this_object(),buy_func,to_buy);
  return 1;
}

void do_buy(object *obs, int cost, object pl) {
  int i;
  mixed fish;

  for (i=0;i<sizeof(obs);i++)
    obs[i]->move(pl);
  pl->pay_money(fish = (int)MONEY_HAND->create_money_array(cost));
  do_parse(buy_mess, obs, pl,
                     (string)MONEY_HAND->money_string(fish), "");
}

int list(string str) {
  object ob;

  if (!test_open())
     return 0;
  if (!str || str == "" || str == "all") {
    if (objectp(our_storeroom))
      ob = our_storeroom;
    else
      ob = find_object(our_storeroom);
    do_parse(list_mess, this_object(), this_player(), "",
                                       shop_list(all_inventory(ob), 0));
    return 1;
  }
  do_parse(list_mess, this_object(), this_player(), "",
           shop_list(find_match(str, our_storeroom), 1));
  return 1;
}

int browse(string str) {
  object *obs;
  int i;

  if (!test_open())
    return 0;
   if (!str || str == "") {
    notify_fail("Usage: browse <objects>\n");
    return 0;
  }

  obs = find_match(str, our_storeroom);
  if (!sizeof(obs)) {
    notify_fail("Cannot find "+str+".\n");
    return 0;
  }
  for (i=0;i<sizeof(obs);i++)
    do_parse(browse_mess, obs[i], this_player(),
         (string)MONEY_HAND->money_value_string(obs[i]->query_value()),
         (string)obs[i]->long());
/*
    write("You look at "+obs[i]->short()+" it costs "+
           MONEY_HAND->money_string(obs[i]->query_money_array())+"\n"+
          obs[i]->long());
 */
  return 1;
}
 int value(string str) {
  object *obs;
  int i, val;

  if (!test_open())
    return 0;

  if(only_sell) {
    tell_object(this_player(),"This shop cannot appraise your goods.\n");
    return 1;
  }
              
  if (!str || str =="") {
    notify_fail("Usage: value <object>\n");
    return 0;
  }

  obs = find_match(str, this_player());
  if (!sizeof(obs)) {
    notify_fail("Cannot find "+str+".\n");
    return 0;
  }
  for (i=0;i<sizeof(obs);i++) {
    /* the call other is so that we can change the PAY_RATES array, and 
       then just update /std/shop to immediately and consistently effect 
       all shops */
    val = obs[i]->query_resale_value();
    
    if(!val) {
      val = (int)"/std/shop"->scaled_value((int)obs[i]->query_value());
      if (val > MAX_AMOUNT)
        val = MAX_AMOUNT;
    }
    
    if(val > obs[i]->query_value())
      val = obs[i]->query_value();

    /* "hot" goods lose value!  */      
    if(obs[i]->query_property("stolen")) {
      if(obs[i]->query_stolen_modifier() == 0)
        val = val * stolen_modifier / 100;
      else
        val = val * obs[i]->query_stolen_modifier() / 100;
    }
      
    do_parse(value_mess, obs[i], this_player(),
             (string)MONEY_HAND->money_value_string(val),
             (string)(obs[i]->do_not_sell() || 
                      (obs[i]->query_resale_value() == -1)) );
  }
  return 1;
}

string shop_list(mixed arr, int detail) {
  mapping inv, costs;
  object *list;
  string s, mon, *shorts, *vals;
  int i, j;
  mixed ind;

  if (pointerp(arr))
    list = arr;
  else
    list = all_inventory(this_object()); 
  /* only keep track of things with shorts ;) */
  inv = ([ ]);
  for (i=0; i<sizeof(list); i++) {
    s = (string)list[i]->short();
    if (!s || !list[i]->query_value())
      continue;
    if(!stringp(s))
      s = "get a creator for this one!";
    if (inv[s])
      inv[s] += ({ list[i] });
    else
      inv[s] = ({ list[i] });
  }
/* ok print it */
  s = "";
  shorts = m_indices(inv);
  if(!sizeof(shorts)) {
    if(detail)
      return "The shop is all out of what you wanted.\n";
    else
      return "The shop is totally out of stock.\n";
  }
  s = "You find on offer:\n";
  for (i=0; i<sizeof(shorts); i++) {
     ind = inv[shorts[i]];
    switch(sizeof(ind)) {
      case 1:
        s += "Our very last " + shorts[i];
        break;
      case 2..5:
        s += capitalize(query_num(sizeof(ind), 0) + " " +
            (string)ind[0]->query_plural());
        break;
      default:
        if(detail)
          s += capitalize(query_num(sizeof(ind), 0) + " " +
              (string)ind[0]->query_plural());
        else
          s += "A large selection of " +
              (string)ind[0]->query_plural();
    }
    if(detail) {
      costs = ([ ]);
      for(j=0;j<sizeof(ind);j++) {
        mon=(string)MONEY_HAND->money_value_string((int)ind[j]->query_value());
        if(!costs[mon])
           costs[mon] = ({ "" + (j + 1) });
        else
          costs[mon] += ({ "" + (j + 1) });
      }
      if(m_sizeof(costs) == 1) {
        s += " for " + m_indices(costs)[0];
        if(sizeof(m_values(costs)[0]) > 1)
          s += " each.\n";
        else
          s += ".\n";
      } else {
        s += ":-\n";
        vals = m_indices(costs);
        for(j=0;j<sizeof(vals);j++)
          s += "  [#" + implode(costs[vals[j]], ",") + "] for " + vals[j] +
              ".\n";
      }
    } else {
      s += ".\n";
    }
  }
  return s;
 }
void set_store_room(mixed ob) {
  if (stringp(ob)) {
    our_storeroom = find_object(ob);
    if (!our_storeroom)
      call_other(ob, "??");
    our_storeroom = find_object(ob);
  }
  our_storeroom = ob;
  our_storeroom->add_property("no_clean_up",1);
}
object query_store_room() { return our_storeroom; }

void do_parse(mixed arr, mixed ob, object client, string money,
                         string extra) {
  if (stringp(arr))
    write(shop_parse(arr, ob, client, money, extra));
  else {
    write(shop_parse(arr[0], ob, client, money, extra));
    say(shop_parse(arr[1], ob, client, money, extra));
  }
}
 string shop_parse(string str, mixed ob, object client, string money,
                         string extra) {
  string s1, s2, s3, rest;

  rest = "";
  while(sscanf(str,"%s$%s$%s", s1, s2, s3) == 3)
    switch (s2) {
      case "ob" :
        if (pointerp(ob))
          str = s1+query_multiple_short(ob)+s3;
        else
          str = s1+ob->short()+s3;
        break;
      case "client" :
        str = s1+client->query_cap_name()+s3;
        break;
      case "extra" :
        str = s1+extra+s3;
        break;
      case "money" :
        str = s1+money+s3;
         break;
      default :
        rest = s1+"$"+s2+"$";
        str = s3;
        break;
    }
  return rest+str;
}


/*
 * The shop types are:
 *  Jewelry
 *  Armory
  *  Magic
 *  General
 */
object *query_stock(string type) {
  mixed *obs;
  int i;

  obs = unique_array(all_inventory(our_storeroom), "query_shop_type");
  for (i=0;i<sizeof(obs);i++)
    if ((string)obs[i][0]->query_shop_type() == type)
      return obs[i];
  return ({ });
}

mixed *stats() {
  return ::stats() + ({
    ({ "total sold", amount_sold }),
     ({ "total bought", amount_bought }),
    });
}
 string query_shop_type() { return shop_type; }
void set_shop_type(string ty) { shop_type = ty; }

 
/* This stuff is for 'sales reps' that run from shop to shop buying and
   selling.  I can't imagine the point.  Leaving it in case someone
   goes crazy and actually wants it.
*/

/* Other shop handling code */
/*
int add_other_shop(mixed shop) {
  mixed *co_ord;

  if (pointerp(shop)) { *//* Given us a co-ordinate *//*
    other_shops += ({ shop });
    return 1;
  }
  co_ord = (mixed *)shop->query_co_ord();
  if (!pointerp(co_ord))
    return 0;
  other_shops += ({ co_ord });
}
*/
/* Ok, now, sales rep creation... Creates a rep  */
/*
object create_rep() {
  object ob;

  ob = clone_object("/obj/monster");
  ob->set_name("rep");
  ob->set_short("sales rep");
  ob->add_adjective("sales");
  ob->set_long("A tall strong looking sales rep.  He stares at you with "
               "bright pierceing eyes.\n");
  ob->set_guild("fighter");
  ob->set_race("human");
  ob->adjust_bon_str(15);  *//* Strong so they can carry all the junk *//*
  ob->set_level(300); *//* Don't want em killed.  They make a lot of money :) *//*
  ARMOURY->request_weapon("bastard", 100)->move(ob);
  ARMOURY->request_weapon("platemail", 100)->move(ob);
  ARMOURY->request_weapon("med_shield", 100)->move(ob);
  ob->init_equip();
   ob->add_property("rep type", shop_type);
}
*/
/* Send them out!  Watch them run! */
/*
void send_out_reps() {
  int i;
  object ob;

  for (i=0;i<sizeof(other_shops);i++) {
    ob = (object)this_object()->create_rep();
    ob->add_property("goto co-ordinate", other_shops[i]);
    ob->add_property("goto property", "shop");
    ob->move(this_object());
    ob->add_triggered_action("froggy", "goto_co_ord", this_object(),
                             "rep_made_it");
  }
}

void rep_made_it(int bing) {
  object *obs;
  int i, cost;
   if (!bing) {
    previous_object()->init_command("'Oh no!  I am utterly lost");
    previous_object()->init_command("sigh");
    call_out("set_up_return", 5, previous_object());
    return ;
  }
  obs = (object *)environment(previous_object())->query_stock(shop_type);
  if (!sizeof(obs)) {
    this_object()->none_to_sell();
    call_out("set_up_return", 5, previous_object());
    return ;
  }
  for (i=0;i<sizeof(obs);i++)
    cost += (int)obs[i]->query_value()*2/3;
  call_out("do_rep_buy", 5, ({ previous_object(), obs, cost }));
  previous_object()->adjust_value(cost);
}
  
void do_rep_buy(mixed *bing) {
  object rep, *obs;
  int i, cost;

  rep = bing[0];
  obs = bing[1];
  cost = bing[2];
  do_buy(obs, cost, rep);
   call_out("set_up_return", 5, rep);
}

void set_up_return(object rep) {
  rep->add_property("goto co-ordinate", this_object()->query_co_ord());
  rep->add_triggered_action("froggy", "goto_co_ord", this_object(),
                             "rep_came_back");
}

int rep_came_back() {
  int i;
  object *obs;

  obs = all_inventory(previous_object());
  for (i=0;i<sizeof(obs);i++)
    obs[i]->move(our_storeroom);
  previous_object()->dest_me();
}
*/
