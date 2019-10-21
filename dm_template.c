/*
 * Author: Yash Gupta <ygupta@ucsc.edu>, Austen Barker <atbarker@ucsc.edu>
 * Copyright: UC Santa Cruz, SSRC
 */
#include <linux/delay.h>
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
    bio_set_dev(bio, context->dev->bdev);

    //Checks if this bio has a nonzero number of sectors or if the operation is a zone reset
    if (bio_sectors(bio) || bio_op(bio) == REQ_OP_ZONE_RESET) {
        bio->bi_iter.bi_sector = context->start + dm_target_offset(ti, bio->bi_iter.bi_sector);
    }

    //submit_bio(bio);
    //return DM_MAPIO_SUBMITTED;
    return DM_MAPIO_REMAPPED;
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
 * @ti Target instance for new device.
 * @return 0  New device instance successfully created.
 * @return <0 Error.
 */
static int
template_ctr(struct dm_target *ti, unsigned int argc, char **argv)
{
    struct template_context *context = NULL;
    int ret = 0;
    sector_t tmp;
    char dummy;

    context = kmalloc(sizeof(*context), GFP_KERNEL);

    ret = dm_get_device(ti, argv[0], dm_table_get_mode(ti->table), &context->dev);
    if (ret) {
        ti->error = "Invalid block device";
	goto error;
    }

    if (sscanf(argv[1], "%lu%c", &tmp, &dummy) != 1 || tmp != (sector_t)tmp) {
        ti->error = "Invalid device sector";
	goto error;
    }
    context->start = tmp;

    return ret;

error:
    kfree(context);
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

    kfree(context);
}

/** ----------------------------------------------------------- DO-NOT-CROSS ------------------------------------------------------------------- **/

static struct target_type template_target = {
    .name = "dmtemplate",
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
