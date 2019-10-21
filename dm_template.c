/*
 * Author: Yash Gupta <ygupta@ucsc.edu>, Austen Barker <atbarker@ucsc.edu>
 * Copyright: UC Santa Cruz, SSRC
 */
#include <linux/delay.h>
#include <lib/bit_vector.h>
#include <linux/bio.h>
#include <linux/completion.h>
#include <linux/device-mapper.h>
#include <linux/kernel.h>
#include <linux/mm_types.h>
#include <linux/module.h>
#include <linux/spinlock_types.h>
#include <linux/stddef.h>
#include <linux/types.h>
#include <linux/version.h>
#include <linux/wait.h>

struct template_context {
    struct dm_dev *dev;
    sector_t start;
};

/**
 * Map function for this target. This is the heart and soul
 * of the device mapper. We receive block I/O requests which
 * we need to remap to our underlying device and then submit
 * the request. This function is essentially called for any I/O 
 * on a device for this target.
 * 
 * The map function is called extensively for each I/O
 * issued upon the device mapper target. For performance 
 * consideration, the map function is verbose only for debug builds.
 * 
 * @ti  Target instance for the device.
 * @bio The block I/O request to be processed.
 * 
 * @return  device-mapper code
 *  DM_MAPIO_SUBMITTED: dm_afs has submitted the bio request.
 *  DM_MAPIO_REMAPPED:  dm_afs has remapped the request and device-mapper
 *                      needs to submit it.
 *  DM_MAPIO_REQUEUE:   dm_afs encountered a problem and the bio needs to
 *                      be resubmitted.
 */
static int
template_map(struct dm_target *ti, struct bio *bio)
{
    struct template_context *context = ti->private;
    int ret = 0;
    if (override) {
        bio_set_dev(bio, context->bdev);
        submit_bio(bio);
        return DM_MAPIO_SUBMITTED;
    }
    return ret;
}

/**
 * Constructor function for this target. The constructor
 * is called for each new instance of a device for this
 * target. To create a new device, 'dmsetup create' is used.
 * 
 * The constructor is used to parse the program arguments
 * and detect the file system in effect on the passive
 * block device.
 * 
 * It also reads all the files in the entropy directory and
 * creates a hash table of the files, keyed by an 8 byte
 * hash.
 * TODO: ^Implement.
 * 
 * @ti Target instance for new device.
 * @return 0  New device instance successfully created.
 * @return <0 Error.
 */
static int
template_ctr(struct dm_target *ti, unsigned int argc, char **argv)
{
    struct template_context *context = NULL;
    int ret;

    context = kmalloc(sizeof(*context), GFP_KERNEL);
    args = &context->args;
    ret = parse_afs_args(args, argc, argv);
    afs_assert(!ret, args_err, "unable to parse arguments");

    // Acquire the block device based on the args. This gives us a
    // wrapper on top of the kernel block device structure.
    ret = dm_get_device(ti, args->dev, dm_table_get_mode(ti->table), &context->dev);
    context->bdev = context->passive_dev->bdev;
    context->config.bdev_size = context->bdev->bd_part->nr_sects;

    return ret;
}

/**
 * Destructor function for this target. The destructor
 * is called when a device instance for this target is
 * destroyed.
 *
 * @ti  Target instance to be destroyed.
 */
static void
template_dtr(struct dm_target *ti)
{
    struct template_context *context = ti->private;

    // Put the device back.
    dm_put_device(ti, context->dev);

    // Free storage used by context.
    kfree(context);
}

/** ----------------------------------------------------------- DO-NOT-CROSS ------------------------------------------------------------------- **/

static struct target_type template_target = {
    .name = "dm-template",
    .version = {0,0,1},
    .module = THIS_MODULE,
    .ctr = template_ctr,
    .dtr = template_dtr,
    .map = template_map
};

/**
 * Initialization function called when the module
 * is inserted dynamically into the kernel. It registers
 * the dm_afs target into the device-mapper tree.
 * 
 * @return  0   Target registered, no errors.
 * @return  <0  Target registration failed.
 */
static __init int
template_init(void)
{
    int ret;

    ret = dm_register_target(&template_target);

    return ret;
}

/**
 * Destructor function called when module is removed
 * from the kernel. This function means nothing when the
 * module is statically linked into the kernel.
 * 
 * Unregisters the dm_afs target from the device-mapper
 * tree.
 */
static void
template_exit(void)
{
    dm_unregister_target(&template_target);
}

module_init(template_init);
module_exit(template_exit);
MODULE_AUTHOR("Austen Barker");
MODULE_LICENSE("GPL");

/** -------------------------------------------------------------------------------------------------------------------------------------------- **/
