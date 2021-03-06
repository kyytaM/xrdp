/**
 * xrdp: A Remote Desktop Protocol server.
 *
 * xrdp device redirection - we mainly use it for drive redirection
 *
 * Copyright (C) Laxmikant Rashinkar 2013
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#if !defined(DEVREDIR_H)
#define DEVREDIR_H

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#include "arch.h"
#include "parse.h"
#include "os_calls.h"
#include "log.h"
#include "chansrv_fuse.h"

typedef struct fuse_data FUSE_DATA;
struct fuse_data
{
    void      *data_ptr;
    FUSE_DATA *next;
};

/* An I/O Resource Packet to track dev_dir I/O calls */

typedef struct irp IRP;

struct irp
{
    tui32      completion_id;       /* unique number                     */
    char       completion_type;     /* describes I/O type                */
    tui32      FileId;              /* RDP client provided unique number */
    //void      *fusep;               /* opaque FUSE info                  */
    char       pathname[256];       /* absolute pathname                 */
    tui32      device_id;           /* identifies remote device          */
    FUSE_DATA *fd_head;             /* point to first FUSE opaque object */
    FUSE_DATA *fd_tail;             /* point to last FUSE opaque object  */
    IRP       *next;                /* point to next IRP                 */
    IRP       *prev;                /* point to previous IRP             */
};

void *dev_redir_fuse_data_peek(IRP *irp);
void *dev_redir_fuse_data_dequeue(IRP *irp);
int   dev_redir_fuse_data_enqueue(IRP *irp, void *vp);

IRP * dev_redir_irp_new();
IRP * dev_redir_irp_find(tui32 completion_id);
IRP * dev_redir_irp_find_by_fileid(tui32 FileId);
IRP * dev_redir_irp_get_last();
int   dev_redir_irp_delete(IRP *irp);
void  dev_redir_irp_dump();

int APP_CC dev_redir_init(void);
int APP_CC dev_redir_deinit(void);

int APP_CC dev_redir_data_in(struct stream* s, int chan_id, int chan_flags,
                             int length, int total_length);

int APP_CC dev_redir_get_wait_objs(tbus* objs, int* count, int* timeout);
int APP_CC dev_redir_check_wait_objs(void);

void dev_redir_send_server_core_cap_req();
void dev_redir_send_server_clientID_confirm();
void dev_redir_send_server_user_logged_on();
void dev_redir_send_server_device_announce_resp(tui32 device_id);

void dev_redir_send_drive_dir_request(IRP *irp, tui32 device_id,
                                      tui32 InitialQuery, char *Path);

int  dev_redir_send_drive_create_request(tui32 device_id, char *path,
                                         tui32 DesiredAccess,
                                         tui32 CreateOptions,
                                         tui32 CreateDisposition,
                                         tui32 completion_id);

int dev_redir_send_drive_close_request(tui16 Component, tui16 PacketId,
                                       tui32 DeviceId,
                                       tui32 FileId,
                                       tui32 CompletionId,
                                       tui32 MajorFunction,
                                       tui32 MinorFunc,
                                       int pad_len);

void dev_redir_proc_client_devlist_announce_req(struct stream *s);
void dev_redir_proc_client_core_cap_resp(struct stream *s);
void dev_redir_proc_device_iocompletion(struct stream *s);

void dev_redir_proc_query_dir_response(IRP *irp,
                                       struct stream *s,
                                       tui32 DeviceId,
                                       tui32 CompletionId,
                                       tui32 IoStatus);

/* misc stuff */
void dev_redir_insert_dev_io_req_header(struct stream *s,
                                        tui32 DeviceId,
                                        tui32 FileId,
                                        tui32 CompletionId,
                                        tui32 MajorFunction,
                                        tui32 MinorFunction);

void devredir_cvt_to_unicode(char *unicode, char *path);
void devredir_cvt_from_unicode_len(char *path, char *unicode, int len);
int  dev_redir_string_ends_with(char *string, char c);

void dev_redir_insert_rdpdr_header(struct stream *s, tui16 Component,
                                   tui16 PacketId);

/* called from FUSE module */
int dev_redir_get_dir_listing(void *fusep, tui32 device_id, char *path);

int dev_redir_file_open(void *fusep, tui32 device_id, char *path,
                        int mode, int type);

int dev_redir_file_read(void *fusep, tui32 device_id, tui32 FileId,
                        tui32 Length, tui64 Offset);

/* module based logging */
#define LOG_ERROR   0
#define LOG_INFO    1
#define LOG_DEBUG   2

#ifndef LOG_LEVEL
#define LOG_LEVEL   LOG_ERROR
#endif

#define log_error(_params...)                           \
{                                                       \
    g_write("[%10.10u]: DEV_REDIR  %s: %d : ERROR: ",   \
            g_time3(), __func__, __LINE__);             \
    g_writeln (_params);                                \
}

#define log_info(_params...)                            \
{                                                       \
    if (LOG_INFO <= LOG_LEVEL)                         \
    {                                                   \
        g_write("[%10.10u]: DEV_REDIR  %s: %d : ",      \
                g_time3(), __func__, __LINE__);         \
        g_writeln (_params);                            \
    }                                                   \
}

#define log_debug(_params...)                           \
{                                                       \
    if (LOG_DEBUG <= LOG_LEVEL)                        \
    {                                                   \
        g_write("[%10.10u]: DEV_REDIR  %s: %d : ",      \
                g_time3(), __func__, __LINE__);         \
        g_writeln (_params);                            \
    }                                                   \
}

int send_channel_data(int chan_id, char *data, int size);

/*
 * RDPDR_HEADER definitions
 */

/* device redirector core component; most of the packets in this protocol */
/* are sent under this component ID                                       */
#define RDPDR_CTYP_CORE                 0x4472

/* printing component. the packets that use this ID are typically about   */
/* printer cache management and identifying XPS printers                  */
#define RDPDR_CTYP_PRN                  0x5052

/* Server Announce Request, as specified in section 2.2.2.2               */
#define PAKID_CORE_SERVER_ANNOUNCE      0x496E

/* Client Announce Reply and Server Client ID Confirm, as specified in    */
/* sections 2.2.2.3 and 2.2.2.6.                                          */
#define PAKID_CORE_CLIENTID_CONFIRM     0x4343

/* Client Name Request, as specified in section 2.2.2.4                   */
#define PAKID_CORE_CLIENT_NAME          0x434E

/* Client Device List Announce Request, as specified in section 2.2.2.9   */
#define PAKID_CORE_DEVICELIST_ANNOUNCE  0x4441

/* Server Device Announce Response, as specified in section 2.2.2.1       */
#define PAKID_CORE_DEVICE_REPLY         0x6472

/* Device I/O Request, as specified in section 2.2.1.4                    */
#define PAKID_CORE_DEVICE_IOREQUEST     0x4952

/* Device I/O Response, as specified in section 2.2.1.5                   */
#define PAKID_CORE_DEVICE_IOCOMPLETION  0x4943

/* Server Core Capability Request, as specified in section 2.2.2.7        */
#define PAKID_CORE_SERVER_CAPABILITY    0x5350

/* Client Core Capability Response, as specified in section 2.2.2.8       */
#define PAKID_CORE_CLIENT_CAPABILITY    0x4350

/* Client Drive Device List Remove, as specified in section 2.2.3.2       */
#define PAKID_CORE_DEVICELIST_REMOVE    0x444D

/* Add Printer Cachedata, as specified in [MS-RDPEPC] section 2.2.2.3     */
#define PAKID_PRN_CACHE_DATA            0x5043

/* Server User Logged On, as specified in section 2.2.2.5                 */
#define PAKID_CORE_USER_LOGGEDON        0x554C

/* Server Printer Set XPS Mode, as specified in [MS-RDPEPC] section 2.2.2.2 */
#define PAKID_PRN_USING_XPS             0x5543

/*
 * Capability header definitions
 */

#define CAP_GENERAL_TYPE   0x0001 /* General cap set - GENERAL_CAPS_SET      */
#define CAP_PRINTER_TYPE   0x0002 /* Print cap set - PRINTER_CAPS_SET        */
#define CAP_PORT_TYPE      0x0003 /* Port cap set - PORT_CAPS_SET            */
#define CAP_DRIVE_TYPE     0x0004 /* Drive cap set - DRIVE_CAPS_SET          */
#define CAP_SMARTCARD_TYPE 0x0005 /* Smart card cap set - SMARTCARD_CAPS_SET */

/* client minor versions */
#define RDP_CLIENT_50                   0x0002
#define RDP_CLIENT_51                   0x0005
#define RDP_CLIENT_52                   0x000a
#define RDP_CLIENT_60_61                0x000c

/* used in device announce list */
#define RDPDR_DTYP_SERIAL               0x0001
#define RDPDR_DTYP_PARALLEL             0x0002
#define RDPDR_DTYP_PRINT                0x0004
#define RDPDR_DTYP_FILESYSTEM           0x0008
#define RDPDR_DTYP_SMARTCARD            0x0020

/*
 * DesiredAccess Mask [MS-SMB2] section 2.2.13.1.1
 */

#define DA_FILE_READ_DATA               0x00000001
#define DA_FILE_WRITE_DATA              0x00000002
#define DA_FILE_APPEND_DATA             0x00000004
#define DA_FILE_READ_EA                 0x00000008 /* rd extended attributes */
#define DA_FILE_WRITE_EA                0x00000010 /* wr extended attributes */
#define DA_FILE_EXECUTE                 0x00000020
#define DA_FILE_READ_ATTRIBUTES         0x00000080
#define DA_FILE_WRITE_ATTRIBUTES        0x00000100
#define DA_DELETE                       0x00010000
#define DA_READ_CONTROL                 0x00020000 /* rd security descriptor */
#define DA_WRITE_DAC                    0x00040000
#define DA_WRITE_OWNER                  0x00080000
#define DA_SYNCHRONIZE                  0x00100000
#define DA_ACCESS_SYSTEM_SECURITY       0x01000000
#define DA_MAXIMUM_ALLOWED              0x02000000
#define DA_GENERIC_ALL                  0x10000000
#define DA_GENERIC_EXECUTE              0x20000000
#define DA_GENERIC_WRITE                0x40000000
#define DA_GENERIC_READ                 0x80000000

/*
 * CreateOptions Mask [MS-SMB2] section 2.2.13 SMB2 CREATE Request
 */

#define CO_FILE_DIRECTORY_FILE          0x00000001
#define CO_FILE_WRITE_THROUGH           0x00000002
#define CO_FILE_SYNCHRONOUS_IO_NONALERT 0x00000020

/*
 * CreateDispositions Mask [MS-SMB2] section 2.2.13
 */

#define CD_FILE_SUPERSEDE               0x00000000
#define CD_FILE_OPEN                    0x00000001
#define CD_FILE_CREATE                  0x00000002
#define CD_FILE_OPEN_IF                 0x00000003
#define CD_FILE_OVERWRITE               0x00000004
#define CD_FILE_OVERWRITE_IF            0x00000005

/*
 * Device I/O Request MajorFunction definitions
 */

#define IRP_MJ_CREATE                   0x00000000
#define IRP_MJ_CLOSE                    0x00000002
#define IRP_MJ_READ                     0x00000003
#define IRP_MJ_WRITE                    0x00000004
#define IRP_MJ_DEVICE_CONTROL           0x0000000E
#define IRP_MJ_QUERY_VOLUME_INFORMATION 0x0000000A
#define IRP_MJ_SET_VOLUME_INFORMATION   0x0000000B
#define IRP_MJ_QUERY_INFORMATION        0x00000005
#define IRP_MJ_SET_INFORMATION          0x00000006
#define IRP_MJ_DIRECTORY_CONTROL        0x0000000C
#define IRP_MJ_LOCK_CONTROL             0x00000011

/*
 * Device I/O Request MinorFunction definitions
 *
 * Only valid when MajorFunction code = IRP_MJ_DIRECTORY_CONTROL
 */

#define IRP_MN_QUERY_DIRECTORY          0x00000001
#define IRP_MN_NOTIFY_CHANGE_DIRECTORY  0x00000002

/*
 * NTSTATUS codes (used by IoStatus)
 */

#define NT_STATUS_SUCCESS               0x00000000
#define NT_STATUS_UNSUCCESSFUL          0xC0000001

/*
 * CompletionID types, used in IRPs to indicate I/O operation
 */

enum
{
    CID_CREATE_DIR_REQ = 1,
    CID_CREATE_OPEN_REQ,
    CID_READ,
    CID_WRITE,
    CID_DIRECTORY_CONTROL,
    CID_CLOSE
};

#if 0
#define CID_CLOSE                       0x0002
#define CID_READ                        0x0003
#define CID_WRITE                       0x0004
#define CID_DEVICE_CONTROL              0x0005
#define CID_QUERY_VOLUME_INFORMATION    0x0006
#define CID_SET_VOLUME_INFORMATION      0x0007
#define CID_QUERY_INFORMATION           0x0008
#define CID_SET_INFORMATION             0x0009
#define CID_DIRECTORY_CONTROL           0x000a
#define CID_LOCK_CONTROL                0x000b
#endif

/*
 * constants for drive dir query
 */

/* Basic information about a file or directory. Basic information is       */
/* defined as the file's name, time stamp, and size, or its attributes     */
#define FileDirectoryInformation        0x00000001

/* Full information about a file or directory. Full information is defined */
/* as all the basic information, plus extended attribute size.             */
#define FileFullDirectoryInformation    0x00000002

/* Basic information plus extended attribute size and short name           */
/* about a file or directory.                                              */
#define FileBothDirectoryInformation    0x00000003

/* Detailed information on the names of files in a directory.              */
#define FileNamesInformation            0x0000000C

/*
 * NTSTATUS Codes of interest to us
 */

/* No more files were found which match the file specification             */
#define STATUS_NO_MORE_FILES            0x80000006

/* Windows file attributes */
#define W_FILE_ATTRIBUTE_DIRECTORY      0x00000010
#define W_FILE_ATTRIBUTE_READONLY       0x00000001

#define WINDOWS_TO_LINUX_FILE_PERM(_a) \
            (((_a) & W_FILE_ATTRIBUTE_DIRECTORY) ? S_IFDIR | 0100 : S_IFREG) |\
            (((_a) & W_FILE_ATTRIBUTE_READONLY)  ? 0444 : 0644)

/* winodws time starts on Jan 1, 1601 */
/* Linux   time starts on Jan 1, 1970 */
#define EPOCH_DIFF 11644473600LL
#define WINDOWS_TO_LINUX_TIME(_t) ((_t) / 10000000) - EPOCH_DIFF;

#endif
