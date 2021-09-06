/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

/*
 * OTA API.
 *
 * See https://mongoose-os.com/docs/mongoose-os/userguide/ota.md for more
 * details about Mongoose OS OTA mechanism.
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "common/mg_str.h"
#include "mgos_event.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MGOS_EVENT_OTA_BASE MGOS_EVENT_BASE('O', 'T', 'A')
enum mgos_event_ota {
  MGOS_EVENT_OTA_BEGIN =
      MGOS_EVENT_OTA_BASE, /* ev_data: struct mgos_ota_begin_arg */
  MGOS_EVENT_OTA_STATUS,   /* ev_data: struct mgos_ota_status */
};

struct mgos_ota_file_info {
  char name[50];
  uint32_t size;
  uint32_t processed;
  uint32_t crc32;
  /* Part that corresponds to this file (JSON). */
  struct mg_str part;
};

struct mgos_ota_manifest_info {
  /* Entire manifest, as JSON. */
  struct mg_str manifest;
  /* Some fields, pre-parsed from the manifest. */
  struct mg_str name;
  struct mg_str platform;
  struct mg_str version;
  struct mg_str build_id;
  struct mg_str parts;
  /*
   * Update signature check results.
   * Bit 0 corresponds to key 0 (update.key), bit 1 to ky 1 (update.key1), etc.
   * E.g. sig_check_result = 1 means signature 0 verified successfully,
   * sig_check_result = 7 means 3 valid signatures by key 0, 1 and 2.
   */
  uint8_t sig_check_result;
};

enum mgos_ota_result {
  MGOS_UPD_WAIT = 0,
  MGOS_UPD_OK,
  MGOS_UPD_SKIP,
  MGOS_UPD_ABORT,
};

struct mgos_ota_begin_arg {
  struct mgos_ota_manifest_info mi;
  /*
   * The default is to continue but handler may set ABORT or WAIT.
   * In case or ABORT, the update is aborted immediately.
   * In case of WAIT, updater will pause the update and periodically
   * re-raise the event until the action is OK or ABORT.
   * Since there can be multiple handlers, be careful:
   *  - If you'd like to continue, don't touch the field at all.
   *  - If you wait to wait, check it's not ABORT (set by somebody else).
   */
  enum mgos_ota_result result;
};

enum mgos_ota_state {
  MGOS_OTA_STATE_IDLE = 0, /* idle */
  MGOS_OTA_STATE_PROGRESS, /* "progress" */
  MGOS_OTA_STATE_ERROR,    /* "error" */
  MGOS_OTA_STATE_SUCCESS,  /* "success" */
};

struct mgos_ota_status {
  bool is_committed;
  int commit_timeout;
  int partition;
  enum mgos_ota_state state;
  const char *msg;      /* stringified state */
  int progress_percent; /* valid only for "progress" state */
};

struct mgos_ota_end_arg {
  int result;
  const char *message;
};

struct mgos_ota_opts {
  int timeout;
  int commit_timeout;
  bool ignore_same_version;
};

const char *mgos_ota_state_str(enum mgos_ota_state state);

void mgos_ota_boot_finish(bool is_successful, bool is_first);
bool mgos_ota_commit();
bool mgos_ota_is_in_progress(void);
bool mgos_ota_is_committed();
bool mgos_ota_is_first_boot(void);
bool mgos_ota_revert(bool reboot);

/*
 * Get status of an update.
 * If there is no update in progress, s->state will be MGOS_OTA_STATE_IDLE.
 */
bool mgos_ota_get_status(struct mgos_ota_status *s);

/*
 * If there is an update in progress, abort it.
 * result is a numerical error code, which must be < 0.
 * msg is an optional message to convey to the user, or NULL.
 *
 * NB: If msg is provided, it must be a constant string
 * or live until MGOS_OTA_STATE_ERROR status is delivered.
 *
 * Returns true if an update was aborted.
 */
bool mgos_ota_abort(int result, const char *msg);

/* Apply update on first boot, usually involves merging filesystem. */
int mgos_ota_apply_update(void);

int mgos_ota_get_commit_timeout(void);
bool mgos_ota_set_commit_timeout(int commit_timeout);

struct mgos_ota_src_if;
struct mgos_ota_src_ctx;

bool mgos_ota_start(const struct mgos_ota_src_if *src_if,
                    struct mgos_ota_src_ctx *src_ctx,
                    const struct mgos_ota_opts *opts);
/*
 * Continue processing, after start. Shoudl be called by source or backend
 * when new data is available or the process in some other way unblocked.
 */
void mgos_ota_process(void);

/*
 * This function is invoked on first boot after update.
 * The default implementation invokes mgos_ota_merge_fs.
 * The function is defined as weak so you can override it with your own.
 */
bool mgos_ota_update_fs(const char *old_fs_path, const char *new_fs_path);

/*
 * Function to merge filesystems.
 * Enumerates files on the old_fs_path and copies them to new_fs_path
 * if mgos_ota_merge_fs_should_copy_file return true.
 */
bool mgos_ota_merge_fs(const char *old_fs_path, const char *new_fs_path);

/*
 * Filter function used by the default implementation of mgos_ota_merge_fs.
 * The default implementation return true unless new_fs_path/file_name
 * already exists.
 * The function is defined as weak so you can override it with your own.
 */
bool mgos_ota_merge_fs_should_copy_file(const char *old_fs_path,
                                        const char *new_fs_path,
                                        const char *file_name);

#ifdef __cplusplus
}
#endif
