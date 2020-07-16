
#include<linux/module.h>
#include<linux/fs.h>
#include<linux/genhd.h>
#include<linux/blkdev.h>
#include<linux/bio.h>
 
 
#define DEVICE_MAJOR    240
#define DEVICE_NAME     "ramdisk"
 
 
#define SECTOR_SIZE 512
//������С

#define DISK_SIZE (3*1024*1024)
//������̴�С

#define SECTOR_ALL (DISK_SIZE/SECTOR_SIZE)
//������̵�������
 
 
static struct gendisk *p_disk; //����ָ�������gendisk�ṹ��
static struct request_queue *p_queue; //����ָ���������
static unsigned char mem_start[DISK_SIZE]; //����һ��3M ���ڴ���Ϊ�������
 

/***********************************************************************
����ram��loop�������豸��ʹ���Լ���д��make_request����������bio
ʡȥʹ���ں�I/O�������Ĺ��̣�make_request����ֵ��0
**********************************************************************/
static void ramdisk_make_request(struct request_queue *q,struct bio *bio)
{
    struct bio_vec *bvec; //bio�ṹ�а������bio_vec�ṹ
    int i; //����ѭ���ı���������Ҫ��ֵ
    void *disk_mem; //ָ������������ڶ�д��λ��
    
    if((bio->bi_sector*SECTOR_SIZE)+bio->bi_size>DISK_SIZE)
    { //��鳬�����������
        printk("ramdisk over flowed!\n");
        bio_endio(bio,1); //�ڶ�������Ϊ1֪ͨ�ں�bio�������
        return ;
    }

    disk_mem=mem_start+bio->bi_sector*SECTOR_SIZE; //mem_start��������̵���ʼ��ַ

    //bio_for_each_segment��һ��forѭ���ĺ꣬ÿ��ѭ������һ��bio_vec
    bio_for_each_segment(bvec,bio,i){
        void *iovec; //ָ���ں˴�����ݵĵ�ַ
        iovec=kmap(bvec->bv_page)+bvec->bv_offset; //��bv_pageӳ�䵽�߶��ڴ�
        switch(bio_data_dir(bio)){ //bio_data_dir(bio)����Ҫ�������ݵķ���
            case READA: //READA��Ԥ����RAED�Ƕ�������ͬһ����
            case READ:memcpy(iovec,disk_mem,bvec->bv_len);break;
            case WRITE:memcpy(disk_mem,iovec,bvec->bv_len);break;
            default:bio_endio(bio,1);kunmap(bvec->bv_page);return ; //����ʧ�ܵ����
        }
        kunmap(bvec->bv_page); //�ͷ�bv_page��ӳ��
        disk_mem+=bvec->bv_len; //�ƶ�������̵�ָ��λ�ã�׼����һ��ѭ��bvec�Ķ�д��׼��
    }
    
    bio_endio(bio,0); //�ڶ�������Ϊ0֪ͨ�ں˴���ɹ�
    return ;
}


static struct block_device_operations ramdisk_fops={
    .owner=THIS_MODULE,
};


static int ramdisk_init(void)
{
    p_queue=blk_alloc_queue(GFP_KERNEL); //����������У�������make_request����
    if(!p_queue)return -1;
    blk_queue_make_request(p_queue, ramdisk_make_request); //���Լ���д��make_request������ӵ�����Ķ���
    p_disk=alloc_disk(1); //����һ��������gendisk�ṹ��
    if(!p_disk)
    {
        blk_cleanup_queue(p_queue); //gendisk����ʧ�ܣ������������������
        return -1;
    }
    strcpy(p_disk->disk_name,DEVICE_NAME); //���豸��
    p_disk->major=DEVICE_MAJOR; //���豸��
    p_disk->first_minor=0; //���豸��
    p_disk->fops=&ramdisk_fops; //fops��ַ
    p_disk->queue=p_queue; //������е�ַ
    set_capacity(p_disk,SECTOR_ALL); //���ô���������
    add_disk(p_disk); //���úú�����������
    return 0;
}


static void ramdisk_exit(void)
{
    del_gendisk(p_disk); //ɾ��gendiskע����Ϣ
    put_disk(p_disk); //�ͷ�disk�ռ�
    blk_cleanup_queue(p_queue); //����������
}
 
 
module_init(ramdisk_init);
module_exit(ramdisk_exit);
MODULE_LICENSE("GPL");

/*

���Բ��裺
lsmodinsmod ramdrv2.ko //��̬��������ģ�� ���ļ����ƣ�
lsmod //�鿴�Ƿ���˸�ramdrv2ģ�飨�ļ����ƣ�
ls /dev //�鿴һ��/devĿ¼���Ƿ���˸�ramdisk�ڵ㣨�����ƴ����ж��壩
mkfs.ext3 /dev/ramdisk //����������Ͻ���ext3�ļ�ϵͳ
mount /dev/ramdisk /mnt/test //���ص�/mnt/testĿ¼���ǲ��Ƕ���lost+found�ļ��У�
mount //�鿴mount�ļ�¼

ж�أ�umount /mnt/testrmmod ramdisk_driver

���ߣ����ܱ�����ӣ�https://www.jianshu.com/p/753e70061037

*/

