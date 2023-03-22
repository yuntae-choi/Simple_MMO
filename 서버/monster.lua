myid = 99999;

function set_uid(x)
   myid = x;
end

function event_player_move(player)
    my_t = API_get_t(myid);
    my_bool = API_og_range(myid, player)
   if (true == my_bool) then
      if (3 == my_t) then
         API_move_npc(myid, player);
      end
   end
end

function event_player_attack(player)
   player_power = API_get_power(player);
   my_bool = API_at_range(myid, player)
   if (true == my_bool) then
       API_Send_Msg(myid, player, player_power*(-1));
       API_attack_npc(myid, player);
       my_hp = API_get_hp(myid);
       API_Send_Msg(myid, player, my_hp);
   end
end

function event_npc_move(player)
    API_Send_Msg(myid, player, "BYE");
end

function event_npc_die(player)
    API_Send_Msg(myid, player, "DIE");
end