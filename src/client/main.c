#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include "xtux.h"
#include "client.h"
#include "win.h"
#include "input.h"
#include "menu.h"
#include "misc.h"
#include "cl_net.h"
#include "entity.h"
#include "image.h"
#include "draw.h"
#include "cl_netmsg_send.h"
#include "particle.h"
#include "text_buffer.h"

extern float sin_lookup[DEGREES]; /*  from ../common/math.c */
extern float cos_lookup[DEGREES];
extern byte num_entity_types; /* entity.c */

map_t *map;
client_t client;
netmsg_entity my_entity;
/* true/false array that records if entity type ALIVE image is loaded
   (used for preloaded when entering map */
static char *ent_img_loaded = NULL;

static void client_init(void);

static void handle_cl_args(int argc, char *argv[]);

int main(int argc, char *argv[])
{
    byte keypress;

    client_init();

    if (argc > 1)    /* Command line arguments */
        handle_cl_args(argc, argv);

    srand(time(NULL));
    calc_lookup_tables(DEGREES);
    win_init();
    entity_init();
    if ((ent_img_loaded = (char *) malloc(num_entity_types)) == NULL)
    {
        perror("Malloc");
        ERR_QUIT("Error allocating ent_img_loaded array", 1);
    } else
    {
        memset(ent_img_loaded, 0, num_entity_types);
    }

    /* Load client preferences from file */
    read_config_file(&client);
    /* Set view dimensions, which are calculated by tile width & heights */
    client.view_w = client.x_tiles * TILE_W;
    client.view_h = client.y_tiles * TILE_H;
    /* What the client WISHES their dimensions were (this may not be what
       it actually is, ie in demo's etc */
    client.desired_w = client.view_w;
    client.desired_h = client.view_h;

    weapon_type_init();
    particle_init();
    menu_init();

    cl_network_init();

    client.state = MENU;

    if (client.demo == DEMO_PLAY)
    {
        cl_network_connect(NULL, 42);
        client.state = GAME_LOAD;
    }

    for (;;)
    {
        switch (client.state)
        {
            case MENU:
                win_ungrab_pointer();
                menu_driver();
                break;

            case GAME_LOAD:
                win_set_properties(VERSION, client.view_w, client.view_h + STATUS_H);
                wait_till_expose(5);

                /* Finished loading, we are ready to play */
                cl_change_map(client.map, client.gamemode);
                break;

            case GAME_RESUME: /* Resume game from menu */
                win_set_properties(VERSION, client.view_w, client.view_h + STATUS_H);
                wait_till_expose(5);
                input_clear();
                draw_status_bar();
                cl_netmsg_send_ready(); /* Tell server new details */
                cl_net_finish(); /* Send the NETMSG_READY to the server */
                client.state = GAME_JOIN;

                break;

            case GAME_JOIN:

                win_set_cursor(client.mousemode != MOUSEMODE_ROTATE);

                if (client.mousemode)
                {
                    win_grab_pointer();
                }

                text_buf_clear();
                particle_clear();

            case GAME_PLAY:
                cl_net_update();

                if (client.state == GAME_PLAY)
                { /* Maybe changed in net_update */
                    draw_crosshair(my_entity);
                    if (client.map_target_active)
                        draw_map_target(my_entity);

                    text_buf_update();
                    if (client.netstats)
                        cl_net_stats();

                    win_update();

                    /* Keyboard Input */
                    client.dir = my_entity.dir;
                    keypress = get_input();
                    cl_netmsg_send_cl_update(keypress, client.dir);
                }

                cl_net_finish(); /* Send things that need sending */
                cap_fps(client.fps);
                break;

            case QUIT:
                game_close();
                image_close(); /* Free all the images */
                win_close();
                write_config_file(&client);
                if (ent_img_loaded != NULL)
                    free(ent_img_loaded);
                exit(EXIT_SUCCESS);
                break;
        }

    }

    return EXIT_SUCCESS;

}

/* Set a bunch of defaults... (client is a global, so defaults to 0 */
static void client_init(void)
{
    client.fps = DEFAULT_FPS;
    client.color1 = COL_ORANGE;
    client.color2 = COL_BLACK;
    strncpy(client.map, DEFAULT_MAP, NETMSG_STRLEN);
    client.health = 100;
    client.player_name = strdup(get_login_name());
    client.movement_mode = NORMAL;
    client.gamemode = SAVETHEWORLD;
    client.sniper_mode = 0;
    client.text_message_display_time = MESSAGE_DISPLAY_TIME;
    client.netstats = NS_NONE;
    client.debug = DEBUG;
    client.crosshair_radius = 100;
    client.turn_rate = 5;
    client.loadscreen = BLUEPRINT;
    client.textentry = 0;
    client.x_tiles = X_TILES;
    client.y_tiles = Y_TILES;
    client.interlace = INTERLACE;
    client.ep_expire = 30;
    client.map_target.x = 1;
    client.map_target.y = 3;
    client.demo = DEMO_NONE;
    client.demoname = NULL;
    client.mousemode = 0;
    client.server_address = NULL;

}


/* Put things back to normal */
void game_close(void)
{

    if (client.connected)
        cl_network_disconnect();
    particle_clear();
    if (map)
        map_close(&map);

}


void preload_entities(char *filename, int gamemode);

extern win_t win;

int cl_change_map(char *map_name, int gamemode)
{

    client.screenpos.x = 0;
    client.screenpos.y = 0;
    client.health = 0; /* To make sure statusbar updates */
    client.map_target_active = 0;

    draw_load_screen(client.loadscreen);

    /* Load the map */
    if (!(map = map_load(client.map, L_BASE | L_OBJECT | L_TOPLEVEL | L_TEXT, MAP)))
    {
        printf(ERR
        "** ERROR LOADING MAP %s! **\n", client.map);
        cl_network_disconnect();
        client.state = MENU; /* Go back to menu */
        return 0;
    }

    /* Load tileset and store pixmap id's in tileset tables  */
    maptext_init(map->text_root);
    tileset_load(map->tileset);
    preload_entities(map_name, gamemode);

    client.state = GAME_JOIN;  /* Status of JOINING on the server */
    cl_netmsg_send_ready();    /* Request to be ACTIVE on the server */
    cl_net_finish();

    /* Clear the map buffer if map doesn't take up the entire buffer as there
       will still be stuff there from the last map */
    if (map->width < client.view_w || map->height < client.view_h)
        clear_area(win.map_buf, 0, 0, client.view_w, client.view_h, "black");
    return 1;

}

static char gamemodechar[NUM_GAME_MODES] = "$%"; /* SAVETHEWORLD, HOLYWAR */

/* Preload images for entities that could be found on this map and gamemode.
   This was basically cut & paste from bits of ../server/sv_map.c */
void preload_entities(char *filename, int gamemode)
{
    char buf[MAXLINE], name[32], *line;
    ent_type_t *et;
    FILE *file;
    int entity_section, i, skipline;

    /* Open the map file */
    if (!(file = open_data_file("maps", filename)))
    {
        printf(ERR
        "%s: Couldn't open %s\n", __FILE__, filename);
        return;
    }

    if (!ent_img_loaded[client.entity_type])
    { /* Load clients entity image */
        if ((et = entity_type(client.entity_type)) == NULL)
            return;
        image(entity_type_animation(et, ALIVE)->pixmap, MASKED);
        ent_img_loaded[client.entity_type] = 1;
    }

    /* Scan map for entities that will be loaded */
    entity_section = 0; /* Found section yet? */
    while (!feof(file))
    {
        if (!fgets(buf, MAXLINE, file))
            break;
        CHOMP(buf);
        if (buf[0] == '#' || buf[0] == '\0')
            continue; /* skip line */
        if (entity_section)
        {
            line = buf;
            skipline = 0;
            /* check for gamemode specific line prefix characters */
            for (i = 0; i < NUM_GAME_MODES; i++)
            {
                if (buf[0] == gamemodechar[i])
                {
                    if (gamemode == i)
                        line = buf + 1;
                    else
                        skipline = 1;
                }
            }

            if (skipline)
                continue; /* Not in our mode */

            if (sscanf(line, "%s", name) && strcasecmp(name, "SPAWN") != 0)
            {
                for (i = 0; i < num_entity_types; i++)
                {
                    if ((et = entity_type(i)) == NULL)
                        continue;
                    if (!ent_img_loaded[i] && !strcasecmp(et->name, name))
                    {
                        ent_img_loaded[i] = 1;
                        image(entity_type_animation(et, ALIVE)->pixmap, MASKED);
                    }
                }
            }
        } else if (!strcasecmp("ENTITY", buf))
            entity_section = 1;
    }

    fclose(file);

}


static void handle_cl_args(int argc, char *argv[])
{
    int i;

    for (i = 1; i < argc; i++)
    {
        if (argv[i][0] == '-')
        {
            switch (argv[i][1])
            {
                case 'p':
                    if (++i < argc)
                    {
                        client.demoname = argv[i];
                        client.demo = DEMO_PLAY;
                    }
                    break;
                case 'r':
                    if (++i < argc)
                    {
                        client.demoname = argv[i];
                        client.demo = DEMO_RECORD;
                    }
                    break;
                case 'h': /* Help (default) */
                default:
                    printf("usage: %s [OPTIONS]\n"
                           "  -p DEMO_NAME Play demo DEMO_NAME\n"
                           "  -r DEMO_NAME Record demo DEMO_NAME\n"
                           "  -h           Display help (this screen)\n\n"
                           "This product is FREE SOFTWARE and comes with "
                           "ABSOLUTELY NO WARRANTY!\n"
                           "Report bugs to philaw@ozemail.com.au\n"
                           "Tho, before you do:\n"
                           "David themselves has stated that XTux is ??dead??*\n"
                           "I don't think they'd care for bug reports anymore.\n"
                           "* https://is.gd/bugdavid\n", argv[0]);
                    exit(EXIT_SUCCESS);
            }
        }
    }

}
