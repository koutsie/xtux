int sv_netmsg_recv_noop(client_t *cl, netmsg msg);

int sv_netmsg_recv_query_version(client_t *cl, netmsg msg);

int sv_netmsg_recv_version(client_t *cl, netmsg msg);

int sv_netmsg_recv_textmessage(client_t *cl, netmsg msg);

int sv_netmsg_recv_quit(client_t *cl, netmsg msg);

int sv_netmsg_recv_join(client_t *cl, netmsg msg);

int sv_netmsg_recv_ready(client_t *cl, netmsg msg);

int sv_netmsg_recv_query_sv_info(client_t *cl, netmsg msg);

int sv_netmsg_recv_cl_update(client_t *cl, netmsg msg);

int sv_netmsg_recv_killme(client_t *cl, netmsg msg);

int sv_netmsg_recv_query_frags(client_t *cl, netmsg msg);

int sv_netmsg_recv_away(client_t *cl, netmsg msg);
