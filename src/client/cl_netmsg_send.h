void cl_netmsg_send_noop(void);

void cl_netmsg_send_query_version(void);

void cl_netmsg_send_version(void);

void cl_netmsg_send_textmessage(char *str);

void cl_netmsg_send_quit(void);

void cl_netmsg_send_join(void);

void cl_netmsg_send_ready(void);

void cl_netmsg_send_query_sv_info(void);

void cl_netmsg_send_cl_update(byte keypress, byte dir);

void cl_netmsg_send_killme(void);

void cl_netmsg_send_query_frags(void);

void cl_netmsg_send_away(void);
