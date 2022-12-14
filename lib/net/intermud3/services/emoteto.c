/*    /daemon/services/emoteto.c
 *    from the Foundation II Object Library
 *    handles the I3 emoteto service
 *    created by Descartes of Borg 950914
 *    Modified for Discworld by Turrican 1-11-95
 */

#include "/net/intermud3/intermud_cfg.h"

#ifdef SERVICE_EMOTETO

void eventReceiveEmote(mixed *packet) {
    object ob;
    string who;

    if( file_name(previous_object()) != INTERMUD_D ) return;
    who = lower_case(packet[5]);
    if( !(ob = find_player(who)) || (int)ob->query_invis() 
#ifndef PLAYER_EMOTETO
        || !ob->query_creator()
#endif
      ) {
    INTERMUD_D->eventWrite(({ "error", 5, mud_name(), 0, packet[2],
                  packet[3], "unk-user", 
                  capitalize(packet[5]) + " is nowhere to "
                    "be found on " + mud_name() + ".",
                  packet }));
    return;
    }
    packet[7] = replace_string(packet[7], "$N", packet[6] + "@" + packet[2]);
    event(this_object(), "player_echo_to",  packet[7]+"\n");
}

void eventSendEmote(string who, string where, string msg) {
    string pl, plc;
    
    pl = (string)this_player(1)->query_name();
    plc = (string)this_player(1)->query_cap_name();
    where = (string)INTERMUD_D->GetMudName(where);
    INTERMUD_D->eventWrite(({ "emoteto", 5, mud_name(), pl, where, 
                  lower_case(who), plc, "$N "+msg }));
}

#endif
