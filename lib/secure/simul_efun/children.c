/* Raskolnikov Nov 96 */

string *block_list = ({
  "/global/demi",
  "/global/demi.c",
  "/global/god",
  "/global/god.c",
});

object *children(string name) {
    int i;
    object *obs; 

    //tc("name: "+identify(name));

    obs = efun::children(name);

    //tc("obs: "+identify(obs));
    if(this_player() && this_player()->query_lord())
    return obs;
    for(i=0;i<sizeof(obs);i++)
    if(member_array(obs[i], block_list))
        if(obs[i]->query_invis() == 2)
        obs -= ({ obs[i] });
    return obs;
}

