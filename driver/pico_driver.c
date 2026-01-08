#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/usb.h>
#include <linux/mutex.h>
#include <linux/uaccess.h> // Necessário para copy_from_user em kernels mais novos

#define VID 0xCAFE
#define PID 0x4001
#define DEVICE_NAME "pico_usb"

// Aumentamos o buffer para suportar SSID + Senha longos
// 512 bytes é seguro para bulk transfer (packet size geralmente é 64, mas o driver fragmenta se necessário)
#define MAX_WRITE_BUFFER 512 

static struct usb_device *pico_dev;
static unsigned char ep_in, ep_out;

static DEFINE_MUTEX(pico_lock);
static struct usb_driver pico_driver;

/* -------------------------------------------------------
 * OPEN / RELEASE
 * -------------------------------------------------------*/

static int pico_open(struct inode *inode, struct file *file)
{
    struct usb_interface *interface;
    int subminor;
    int retval = 0;

    subminor = iminor(inode);

    /* Busca a interface associada ao minor number */
    interface = usb_find_interface(&pico_driver, subminor);
    if (!interface) {
        pr_err("pico_usb: erro - nao foi possivel encontrar interface\n");
        return -ENODEV;
    }

    /* Salva a interface no arquivo para usar depois */
    file->private_data = interface;

    /* ACORDA O DISPOSITIVO (Autoresume) */
    retval = usb_autopm_get_interface(interface);
    if (retval) {
        pr_err("pico_usb: falha no autoresume (%d)\n", retval);
        return retval;
    }

    /* Confirma conexão */
    if (!pico_dev) {
        usb_autopm_put_interface(interface); // Desfaz o get se falhar
        return -ENODEV;
    }

    pr_info("pico_usb: device opened (autoresume ok)\n");
    return 0;
}

static int pico_release(struct inode *inode, struct file *file)
{
    struct usb_interface *interface = file->private_data;

    /* PERMITE QUE O DISPOSITIVO DURMA DE NOVO */
    if (interface)
        usb_autopm_put_interface(interface);

    pr_info("pico_usb: device closed\n");
    return 0;
}

/* -------------------------------------------------------
 * WRITE
 * -------------------------------------------------------*/
static ssize_t pico_write(struct file *f, const char __user *buf,
                          size_t cnt, loff_t *ppos)
{
    char kbuf[MAX_WRITE_BUFFER]; // Buffer aumentado
    int ret, actual;
    
    // Log apenas para debug (pode remover em produção para não poluir dmesg)
    // pr_info("pico_usb: write request of %zu bytes\n", cnt);

    // Proteção contra buffer overflow
    if (cnt > MAX_WRITE_BUFFER)
    {
        pr_warn("pico_usb: dados truncados de %zu para %d bytes\n", cnt, MAX_WRITE_BUFFER);
        cnt = MAX_WRITE_BUFFER;
    }

    // Copia dados do usuário (App C) para o Kernel Space
    if (copy_from_user(kbuf, buf, cnt))
    {
        ret = -EFAULT;
        pr_err("pico_usb: Erro copy_from_user\n");
        return ret;
    }

    // Envio via USB Bulk
    mutex_lock(&pico_lock);
    
    // O timeout foi aumentado levemente para 5000ms para garantir estabilidade no Wi-Fi
    ret = usb_bulk_msg(pico_dev,
                       usb_sndbulkpipe(pico_dev, ep_out),
                       kbuf, cnt, &actual, 5000);
    mutex_unlock(&pico_lock);

    if (ret)
    {
        pr_err("pico_usb: Erro usb_bulk_msg: %d\n", ret);
        return ret;
    }

    return actual;
}

static const struct file_operations pico_fops = {
    .owner = THIS_MODULE,
    .open = pico_open,
    .release = pico_release,
    .write = pico_write,
};

static struct usb_class_driver pico_class = {
    .name = "pico_usb",
    .fops = &pico_fops,
    .minor_base = 192,
};

/* -------------------------------------------------------
 * PROBE / DISCONNECT
 * -------------------------------------------------------*/

static int pico_probe(struct usb_interface *intf,
                      const struct usb_device_id *id)
{
    struct usb_host_interface *iface_desc;
    struct usb_endpoint_descriptor *ep;
    int i;
    int retval; // <--- Variável nova

    pico_dev = usb_get_dev(interface_to_usbdev(intf));
    iface_desc = intf->cur_altsetting;

    ep_in = ep_out = 0;

    for (i = 0; i < iface_desc->desc.bNumEndpoints; i++) {
        ep = &iface_desc->endpoint[i].desc;
        if (!ep_in && usb_endpoint_is_bulk_in(ep))
            ep_in = ep->bEndpointAddress;
        if (!ep_out && usb_endpoint_is_bulk_out(ep))
            ep_out = ep->bEndpointAddress;
    }

    if (!ep_in || !ep_out) {
        printk(KERN_ERR "pico_usb: Endpoints não encontrados\n");
        return -ENODEV;
    }

    /* TRUQUE: Incrementa o contador de uso de energia. 
       Enquanto não dermos "put", o Kernel NÃO suspende a placa. */
    retval = usb_autopm_get_interface(intf);
    if (retval < 0) {
        dev_err(&intf->dev, "pico_usb: falha ao dar resume (%d)\n", retval);
        return retval;
    }

    usb_register_dev(intf, &pico_class);
    printk(KERN_INFO "Pico USB conectado (Autosuspend bloqueado)\n");
    return 0;
}

static void pico_disconnect(struct usb_interface *intf)
{
    usb_deregister_dev(intf, &pico_class);
    
    /* Libera o contador de energia ao remover */
    usb_autopm_put_interface(intf);
    
    printk(KERN_INFO "Pico USB desconectado\n");
}

static const struct usb_device_id pico_id_table[] = {
    { USB_DEVICE(VID, PID) },
    {}
};
MODULE_DEVICE_TABLE(usb, pico_id_table);

static struct usb_driver pico_driver = {
    .name = "pico_usb",
    .probe = pico_probe,
    .disconnect = pico_disconnect,
    .id_table = pico_id_table,
    .supports_autosuspend = 1,
};

module_usb_driver(pico_driver);

MODULE_LICENSE("GPL");