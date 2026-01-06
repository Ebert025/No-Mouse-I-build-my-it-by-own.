#include <X11/Xlib.h>
#include <linux/input.h>
#include <stdio.h>
#include <libudev.h>
#include <libinput.h>
#include <memory.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdbool.h>
#include <fcntl.h>
#include <X11/extensions/XTest.h>
#include <linux/input-event-codes.h>
#include <linux/uinput.h>
#include <X11/keysym.h>


/* compilar via : gcc main.c -o app -fsanitize=address -g -linput -ludev -lc && sudo ./app */

typedef enum {
    MOUSE_LEFT=0,
    MOUSE_RIGHT,
    MOUSE_UP,
    MOUSE_DOWN,
    MOUSE_BUTTON_LEFT,
    MOUSE_BUTTON_RIGHT,
    MOUSE_SPECIAL, // não exatamente um butão, dev-stuff
    MOUSE_SCROLL_UP,
    MOUSE_SCROLL_DOWN,
    MOUSE_SCROLL_LEFT,
    MOUSE_SCROLL_RIGHT,
    _MOUSE_LAST_ITEM
} mouse_act;
typedef enum {
    CONF_SENSIBILITY=0,
    CONF_DELAY_IN_MICROSECONDS,
    CONF_WHELL,
    CONF_NUMPADCONSUME,
    _CONF_LAST_ITEM
} config_item;

static struct libinput_event* event;
static struct libinput* input;
static int fd;
static mouse_act action[_MOUSE_LAST_ITEM] = {};
static long configs[_CONF_LAST_ITEM] = {};
static bool running=true;
static Display* monitor;
static Window root;


void inputloop();
void keyboardinput(struct libinput_event* event);
void setupdev();
void dev_sync();
/* [X or Y] -> true = X && false = Y */
void dev_sendxy(int rel,bool X_or_Y);
void dev_sendclick(bool pressed,bool LEFT_or_RIGHT); // click direito e esquerdo
void dev_sendscroll(int rel, bool VERTICAL_or_HORIZONTAL); // scroll horizontal e vertical
void process(); // processar eventos do teclado
int inputopen(const char *path, int flags, void *user_data);
void inputclose(int fd, void *user_data);
int translate_c2k(char c);
void reconfig();
void consume_numpad(); // tirar "input" do numpad, assim quando pressiona algo nele tipo numpad '8' ele nn digita um 8 acidentalmente
void unconsume_numpad();


int main() {
    if (geteuid()!=0) return -printf("No root :(\n"); // vira um -12 de código de erro mas tanto faz.

    printf("SUPER [Keyboard2Mouse-Driver] APP!\n(feito-by-ebert)\n\n");
    printf("Controles: (Numpad-apenas)\n + [+]Whell-sensi\t- [-]Whell-sensi\n");
    printf(" 7 Mouse-left-Click\t8 Mouse-Up\t9 Mouse-right-Click\n");
    printf(" 4 Mouse-left\t5 Special key\t6 Mouse-right\n");
    printf(" 1 [+]Mouse-sensi\t2 Mouse-Down\t3 [-]Mouse-sensi\n\n");
    printf("[Special] + [+]Mouse-sensi = [+100]Delay-microsecond\n[Special] + [-]Mouse-sensi = [-100]Delay-microsecond\n[Special] + [Any-mouse-move-key] = Scroll(either VERTICAL or HORIZONTAL)\n[Special] + Numlock = Consume or Unconsume numpad(free for others apps)\n\n");

    /* segurança apenas, Nn confio total no "statement = {}" pra zerar */
    memset(configs, 0, sizeof(configs));
    memset(action,0,sizeof(action));
    /* defaults */
    configs[CONF_SENSIBILITY] = 1; // 1 pixel por movimento
    configs[CONF_DELAY_IN_MICROSECONDS] = 5000; // 5 milisegundos de delay
    configs[CONF_WHELL] = 4; // 4 pixeis por pressiona no scroll
    configs[CONF_NUMPADCONSUME] = 1;

    reconfig();

    monitor = XOpenDisplay(NULL);
    root = XDefaultRootWindow(monitor);
    consume_numpad();

    struct udev* dev = udev_new();
    struct libinput_interface inputiface;
    inputiface.open_restricted=&inputopen;
    inputiface.close_restricted=&inputclose;

    input = libinput_udev_create_context(&inputiface,NULL,dev);
    libinput_udev_assign_seat(input, "seat0"); /* conjuto default do meu usuario de devices: teclado0, mouse0, monitor0 fds*/

    fd = open("/dev/uinput",O_WRONLY|O_NONBLOCK);

    setupdev();

    printf("CTRL-C ou NUMPAD-Enter para sair...\n");
    while (running) inputloop(); /* never nesting + i'm a functional brother :) */
    printf("Bye!\n");

    libinput_unref(input);
    udev_unref(dev);
    close(fd);
    XCloseDisplay(monitor);
    return 0;
}

void dev_sendxy(int rel,bool X_or_Y) {
    struct input_event ev;
    memset(&ev,0,sizeof(ev));
    ev.value=rel;
    ev.type=EV_REL;
    ev.code=X_or_Y ? REL_X : REL_Y;
    write(fd,&ev,sizeof(ev));
    dev_sync();
}
void dev_sendclick(bool pressed,bool LEFT_or_RIGHT) {
    struct input_event ev;
    memset(&ev,0,sizeof(ev));
    ev.value=pressed;
    ev.type=EV_KEY;
    ev.code=LEFT_or_RIGHT ? BTN_LEFT : BTN_RIGHT;
    write(fd,&ev,sizeof(ev));
    dev_sync();
}
void dev_sendscroll(int rel, bool VERTICAL_or_HORIZONTAL) {
    struct input_event ev;
    memset(&ev,0,sizeof(ev));
    ev.value=rel;
    ev.type=EV_REL;
    ev.code=VERTICAL_or_HORIZONTAL ? REL_WHEEL : REL_HWHEEL;
    write(fd,&ev,sizeof(ev));
    dev_sync();
}

// void dev_sendkey(int key) {
//     KeyCode code = XKeysymToKeycode(monitor, key);
//     XTestFakeKeyEvent(monitor, code, True, CurrentTime);   // press
//     XFlush(monitor);
//     XTestFakeKeyEvent(monitor, code, False, CurrentTime);  // release
//     XFlush(monitor);
// }

void dev_sync() {
    struct input_event ev;
    memset(&ev,0,sizeof(ev));
    ev.type=EV_SYN;
    ev.code = SYN_REPORT;
    // ev.value = 0
    write(fd,&ev,sizeof(ev));
}

void setupdev() {
    ioctl(fd,UI_SET_EVBIT,EV_REL); // mov relativo
    ioctl(fd, UI_SET_RELBIT, REL_X);
    ioctl(fd, UI_SET_RELBIT, REL_Y);
    ioctl(fd, UI_SET_RELBIT, REL_WHEEL); // SCROLL!!!
    ioctl(fd, UI_SET_RELBIT, REL_HWHEEL);
    ioctl(fd, UI_SET_EVBIT, EV_KEY); // botões
    ioctl(fd, UI_SET_KEYBIT, BTN_LEFT);
    ioctl(fd, UI_SET_KEYBIT, BTN_RIGHT);



    
    struct uinput_user_dev dev;
    memset(&dev, 0, sizeof(dev));
    snprintf(dev.name,UINPUT_MAX_NAME_SIZE,"KBD2MOUSE VIRT");
    dev.id.bustype = BUS_USB;
    dev.id.version = 2;
    dev.id.vendor = 0xEBE4;
    dev.id.product = 0x4441;
    dev.ff_effects_max=0;
    write(fd,&dev,sizeof(dev));
    ioctl(fd, UI_DEV_CREATE);
}

void inputloop() {
    libinput_dispatch(input);
    struct libinput_event* event = libinput_get_event(input);
    if (!event) return process();

    if (libinput_event_get_type(event)==LIBINPUT_EVENT_KEYBOARD_KEY) keyboardinput(event);

    libinput_event_destroy(event);
}
void process() {
    if (action[MOUSE_UP]) dev_sendxy(-configs[CONF_SENSIBILITY],false);
    if (action[MOUSE_DOWN]) dev_sendxy(configs[CONF_SENSIBILITY],false);
    if (action[MOUSE_LEFT]) dev_sendxy(-configs[CONF_SENSIBILITY],true);
    if (action[MOUSE_RIGHT]) dev_sendxy(configs[CONF_SENSIBILITY],true);
    /* movimento da roda do mouse é bem rápido mesmo no 1 e como não suporta double, por isso não fica aqui :P */
    if (action[MOUSE_BUTTON_LEFT]) dev_sendclick(true,true);
    if (action[MOUSE_BUTTON_RIGHT]) dev_sendclick(true,false);
    /* como ajusto lá no keyboardinput() não precisa num if ficar constantemente dando dev_sendXclick(false) */
    usleep(configs[CONF_DELAY_IN_MICROSECONDS]);
}

/* porcausa do jeito que a libinput faz os events usar numpad 7 + 8 por exemplo vira um segurando e arrastando pra cima safe sem precisa implementar */
void keyboardinput(struct libinput_event* event) {
    struct libinput_event_keyboard* k=libinput_event_get_keyboard_event(event);
    uint32_t key = libinput_event_keyboard_get_key(k);
    enum libinput_key_state state = libinput_event_keyboard_get_key_state(k);
    
    if (state==LIBINPUT_KEY_STATE_RELEASED) goto release;
    /* Pressionando */ 
    

    
    
    if (key==KEY_KP8&&action[MOUSE_SPECIAL]) printf("[Keyboard2Mouse] Scroll-up\n"),action[MOUSE_SCROLL_UP]=true,dev_sendscroll(configs[CONF_WHELL], true);
    else if (key==KEY_KP2&&action[MOUSE_SPECIAL]) printf("[Keyboard2Mouse] Scroll-down\n"),action[MOUSE_SCROLL_DOWN]=true,dev_sendscroll(-configs[CONF_WHELL], true);
    else if (key==KEY_KP4&&action[MOUSE_SPECIAL]) printf("[Keyboard2Mouse] Scroll-left\n"),action[MOUSE_SCROLL_LEFT]=true,dev_sendscroll(-configs[CONF_WHELL], false);
    else if (key==KEY_KP6&&action[MOUSE_SPECIAL]) printf("[Keyboard2Mouse] Scroll-right\n"),action[MOUSE_SCROLL_RIGHT]=true,dev_sendscroll(configs[CONF_WHELL], false);
    
    else if (key==KEY_KP8) printf("[Keyboard2Mouse] Cima\n"),action[MOUSE_UP]=true;
    else if (key==KEY_KP2) printf("[Keyboard2Mouse] Baixo\n"),action[MOUSE_DOWN]=true;
    else if (key==KEY_KP4) printf("[Keyboard2Mouse] Esquerda\n"),action[MOUSE_LEFT]=true;
    else if (key==KEY_KP6) printf("[Keyboard2Mouse] Direita\n"),action[MOUSE_RIGHT]=true;
    else if (key==KEY_KP7) printf("[Keyboard2Mouse] Click-Esquerdo\n"),action[MOUSE_BUTTON_LEFT]=true;
    else if (key==KEY_KP9) printf("[Keyboard2Mouse] Click-Direito\n"),action[MOUSE_BUTTON_RIGHT]=true;
    
    else if (key==KEY_NUMLOCK&&action[MOUSE_SPECIAL]) configs[CONF_NUMPADCONSUME] ? unconsume_numpad() : consume_numpad(),configs[CONF_NUMPADCONSUME]=!configs[CONF_NUMPADCONSUME], printf("[Keyboard2Mouse] NumpadConsume = %ld\n",configs[CONF_NUMPADCONSUME]);
    else if (key==KEY_KP1&&action[MOUSE_SPECIAL]) configs[CONF_DELAY_IN_MICROSECONDS]+=100,printf("[Keyboard2Mouse] [+] delay microseconds = %ld\n",configs[CONF_DELAY_IN_MICROSECONDS]);
    else if (key==KEY_KP3&&action[MOUSE_SPECIAL]&&configs[CONF_DELAY_IN_MICROSECONDS]>100) configs[CONF_DELAY_IN_MICROSECONDS]-=100,printf("[Keyboard2Mouse] [-] delay microseconds = %ld\n",configs[CONF_DELAY_IN_MICROSECONDS]);
    else if (key==KEY_KPPLUS) printf("[Keyboard2Mouse] [+] Whell = %ld\n",++configs[CONF_WHELL]);
    else if (key==KEY_KPMINUS&&configs[CONF_WHELL]>1) printf("[Keyboard2Mouse] [-] Whell = %ld\n",--configs[CONF_WHELL]);
    else if (key==KEY_KP5) printf("[Keyboard2Mouse] {SPECIAL}\n"),action[MOUSE_SPECIAL]=true;
    else if (key==KEY_KPENTER) running=false;
    else if (key==KEY_KP1) printf("[Keyboard2Mouse] [+] sensibility = %ld\n",++configs[CONF_SENSIBILITY]);
    else if (key==KEY_KP3&&configs[CONF_SENSIBILITY]>1) printf("[Keyboard2Mouse] [-] sensibility = %ld\n",--configs[CONF_SENSIBILITY]);
    return;


    return;

    release: /* Soltando */
    
    if (key==KEY_KP8&&action[MOUSE_SPECIAL]) action[MOUSE_SCROLL_UP]=false;
    else if (key==KEY_KP4&&action[MOUSE_SPECIAL]) action[MOUSE_SCROLL_LEFT]=false;
    else if (key==KEY_KP6&&action[MOUSE_SPECIAL]) action[MOUSE_SCROLL_RIGHT]=false;
    else if (key==KEY_KP2&&action[MOUSE_SPECIAL]) action[MOUSE_SCROLL_DOWN]=false;
    else if (key==KEY_KP8) action[MOUSE_UP]=false;
    else if (key==KEY_KP2) action[MOUSE_DOWN]=false;
    else if (key==KEY_KP4) action[MOUSE_LEFT]=false;
    else if (key==KEY_KP6) action[MOUSE_RIGHT]=false;
    else if (key==KEY_KP5) action[MOUSE_SPECIAL]=false,action[MOUSE_SCROLL_LEFT]=false,action[MOUSE_SCROLL_RIGHT]=false,action[MOUSE_SCROLL_UP]=false,action[MOUSE_SCROLL_DOWN]=false;
    else if (key==KEY_KP7) action[MOUSE_BUTTON_LEFT]=false,dev_sendclick(false,true); // só pra ficilitar stuff
    else if (key==KEY_KP9) action[MOUSE_BUTTON_RIGHT]=false,dev_sendclick(false,false);
    return;
    release_kbd:

    return;
}


void reconfig() {
    printf("Mudar configuração? Default(sensi=%ld pixeis,delay=%ld microseconds,whell=%ld) (s/n)\n",configs[CONF_SENSIBILITY],configs[CONF_DELAY_IN_MICROSECONDS],configs[CONF_WHELL]);
    char c=getchar();
    if (c!='s'&&c!='S') return;
    printf("formato: '(sensi) (delay_em_microsegundos) (whell)'\n");
    scanf("%ld %ld %ld",&configs[CONF_SENSIBILITY],&configs[CONF_DELAY_IN_MICROSECONDS],&configs[CONF_WHELL]);
}

void consume_numpad() {
    /* Bem improvavel algum app tá usando e se tiver nn tem muito problema */
    XGrabKey(monitor, XKeysymToKeycode(monitor, XK_KP_0), AnyModifier, root, True, GrabModeAsync,GrabModeAsync);
    XGrabKey(monitor, XKeysymToKeycode(monitor, XK_KP_Enter), AnyModifier, root, True, GrabModeAsync,GrabModeAsync);
    XGrabKey(monitor, XKeysymToKeycode(monitor, XK_KP_1), AnyModifier, root, True, GrabModeAsync,GrabModeAsync);
    XGrabKey(monitor, XKeysymToKeycode(monitor, XK_KP_2), AnyModifier, root, True, GrabModeAsync,GrabModeAsync);
    XGrabKey(monitor, XKeysymToKeycode(monitor, XK_KP_3), AnyModifier, root, True, GrabModeAsync,GrabModeAsync);
    XGrabKey(monitor, XKeysymToKeycode(monitor, XK_KP_4), AnyModifier, root, True, GrabModeAsync,GrabModeAsync);
    XGrabKey(monitor, XKeysymToKeycode(monitor, XK_KP_5), AnyModifier, root, True, GrabModeAsync,GrabModeAsync); // [SPECIAL] KEY!! 5 + [any move key] = SCROLL)! 5 + [sensi-keys] = DELAY CHANGE!
    XGrabKey(monitor, XKeysymToKeycode(monitor, XK_KP_6), AnyModifier, root, True, GrabModeAsync,GrabModeAsync);
    XGrabKey(monitor, XKeysymToKeycode(monitor, XK_KP_7), AnyModifier, root, True, GrabModeAsync,GrabModeAsync);
    XGrabKey(monitor, XKeysymToKeycode(monitor, XK_KP_8), AnyModifier, root, True, GrabModeAsync,GrabModeAsync);
    XGrabKey(monitor, XKeysymToKeycode(monitor, XK_KP_9), AnyModifier, root, True, GrabModeAsync,GrabModeAsync);
    XGrabKey(monitor, XKeysymToKeycode(monitor, XK_KP_Add), AnyModifier, root, True, GrabModeAsync,GrabModeAsync);
    XGrabKey(monitor, XKeysymToKeycode(monitor, XK_KP_Subtract), AnyModifier, root, True, GrabModeAsync,GrabModeAsync);
    XGrabKey(monitor, XKeysymToKeycode(monitor, XK_Num_Lock), AnyModifier, root, True, GrabModeAsync, GrabModeAsync);
    XFlush(monitor);
}
void unconsume_numpad() {
    XUngrabKey(monitor, XKeysymToKeycode(monitor, XK_KP_Enter), AnyModifier, root);
    XUngrabKey(monitor, XKeysymToKeycode(monitor, XK_KP_0), AnyModifier, root);
    XUngrabKey(monitor, XKeysymToKeycode(monitor, XK_KP_1), AnyModifier, root);
    XUngrabKey(monitor, XKeysymToKeycode(monitor, XK_KP_2), AnyModifier, root);
    XUngrabKey(monitor, XKeysymToKeycode(monitor, XK_KP_3), AnyModifier, root);
    XUngrabKey(monitor, XKeysymToKeycode(monitor, XK_KP_4), AnyModifier, root);
    XUngrabKey(monitor, XKeysymToKeycode(monitor, XK_KP_5), AnyModifier, root); 
    XUngrabKey(monitor, XKeysymToKeycode(monitor, XK_KP_6), AnyModifier, root);
    XUngrabKey(monitor, XKeysymToKeycode(monitor, XK_KP_7), AnyModifier, root);
    XUngrabKey(monitor, XKeysymToKeycode(monitor, XK_KP_8), AnyModifier, root);
    XUngrabKey(monitor, XKeysymToKeycode(monitor, XK_KP_9), AnyModifier, root);
    XUngrabKey(monitor, XKeysymToKeycode(monitor, XK_KP_Add), AnyModifier, root);
    XUngrabKey(monitor, XKeysymToKeycode(monitor, XK_KP_Subtract), AnyModifier, root);
    XUngrabKey(monitor, XKeysymToKeycode(monitor, XK_Num_Lock), AnyModifier, root);
    XFlush(monitor);
}

int inputopen(const char *path, int flags, void *user_data) {
    return open(path,flags|O_CLOEXEC);
}

void inputclose(int fd, void *user_data) {
    close(fd);
}

