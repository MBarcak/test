/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2020  IBM Corporation.
 *
 *  GRUB is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  GRUB is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GRUB.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <grub/time.h>
#include <grub/misc.h>
#include <grub/dl.h>
#include <grub/command.h>
#include <grub/env.h>
#include <grub/test.h>
#include <grub/mm.h>
#include <grub/procfs.h>
#include <grub/file.h>

#include "appended_signatures.h"

GRUB_MOD_LICENSE ("GPLv3+");

#define PROC_FILE(identifier, file_name) \
static char * \
get_ ## identifier (grub_size_t *sz) \
{ \
  char *ret; \
  \
  *sz = identifier ## _len; \
  ret = grub_malloc (*sz); \
  if (ret) \
    grub_memcpy (ret, identifier, *sz); \
  return ret; \
} \
\
static struct grub_procfs_entry identifier ## _entry = \
{ \
  .name = file_name, \
  .get_contents = get_ ## identifier \
};

#define DEFINE_TEST_CASE(case_name) PROC_FILE(case_name, #case_name)

#define DO_TEST(case_name, is_valid) \
{ \
  grub_procfs_register (#case_name, &case_name ## _entry); \
  do_verify ("(proc)/" #case_name, is_valid); \
  grub_procfs_unregister (&case_name ## _entry); \
}


DEFINE_TEST_CASE (hi_signed);
DEFINE_TEST_CASE (hi_signed_sha256);
DEFINE_TEST_CASE (hj_signed);
DEFINE_TEST_CASE (short_msg);
DEFINE_TEST_CASE (unsigned_msg);
DEFINE_TEST_CASE (hi_signed_2nd);
DEFINE_TEST_CASE (hi_double);
DEFINE_TEST_CASE (hi_double_extended);

PROC_FILE (certificate_der, "certificate.der")
PROC_FILE (certificate2_der, "certificate2.der")
PROC_FILE (certificate_printable_der, "certificate_printable.der")
PROC_FILE (certificate_eku_der, "certificate_eku.der")

static void
do_verify (const char *f, int is_valid)
{
  grub_command_t cmd;
  char *args[] = { (char *) f, NULL };
  grub_err_t err;

  cmd = grub_command_find ("verify_appended");
  if (!cmd)
    {
      grub_test_assert (0, "can't find command `%s'", "verify_appended");
      return;
    }
  err = (cmd->func) (cmd, 1, args);
  if (is_valid)
    {
      grub_test_assert (err == GRUB_ERR_NONE,
			"verification of %s failed: %d: %s", f, grub_errno,
			grub_errmsg);
    }
  else
    {
      grub_test_assert (err == GRUB_ERR_BAD_SIGNATURE,
			"verification of %s unexpectedly succeeded", f);
    }
  grub_errno = GRUB_ERR_NONE;

}

static void
appended_signature_test (void)
{
  grub_command_t cmd_trust, cmd_distrust;
  char *trust_args[] = { (char *) "(proc)/certificate.der", NULL };
  char *trust_args2[] = { (char *) "(proc)/certificate2.der", NULL };
  char *trust_args_printable[] = { (char *) "(proc)/certificate_printable.der",
				   NULL };
  char *trust_args_eku[] = { (char *) "(proc)/certificate_eku.der", NULL };
  char *distrust_args[] = { (char *) "1", NULL };
  char *distrust2_args[] = { (char *) "2", NULL };
  grub_err_t err;

  grub_procfs_register ("certificate.der", &certificate_der_entry);
  grub_procfs_register ("certificate2.der", &certificate2_der_entry);
  grub_procfs_register ("certificate_printable.der",
			&certificate_printable_der_entry);
  grub_procfs_register ("certificate_eku.der", &certificate_eku_der_entry);

  cmd_trust = grub_command_find ("trust_certificate");
  if (!cmd_trust)
    {
      grub_test_assert (0, "can't find command `%s'", "trust_certificate");
      return;
    }
  err = (cmd_trust->func) (cmd_trust, 1, trust_args);

  grub_test_assert (err == GRUB_ERR_NONE,
		    "loading certificate failed: %d: %s", grub_errno,
		    grub_errmsg);

  /* If we have no certificate the remainder of the tests are meaningless */
  if (err != GRUB_ERR_NONE)
    return;

  /*
   * Reload the command: this works around some 'interesting' behaviour in the
   * dynamic command dispatcher. The first time you call cmd->func you get a
   * dispatcher that loads the module, finds the real cmd, calls it, and then
   * releases some internal storage. This means it's not safe to call a second
   * time and we need to reload it.
   */
  cmd_trust = grub_command_find ("trust_certificate");

  /* hi, signed with key 1, SHA-512 */
  DO_TEST (hi_signed, 1);

  /* hi, signed with key 1, SHA-256 */
  DO_TEST (hi_signed_sha256, 1);

  /* hi, key 1, SHA-512, second byte corrupted */
  DO_TEST (hj_signed, 0);

  /* message too short for a signature */
  DO_TEST (short_msg, 0);

  /* lorem ipsum */
  DO_TEST (unsigned_msg, 0);

  /* hi, signed with both keys, SHA-512 */
  DO_TEST (hi_double, 1);

  /*
   * hi, signed with both keys and with empty space to test we haven't broken
   * support for adding more signatures after the fact
   */
  DO_TEST (hi_double_extended, 1);

  /*
   * in enforcing mode, we shouldn't be able to load a certificate that isn't
   * signed by an existing trusted key.
   *
   * However, procfs files automatically skip the verification test, so we can't
   * easily test this.
   */

  /*
   * verify that testing with 2 trusted certs works
   */
  DO_TEST (hi_signed_2nd, 0);

  err = (cmd_trust->func) (cmd_trust, 1, trust_args2);

  grub_test_assert (err == GRUB_ERR_NONE,
		    "loading certificate 2 failed: %d: %s", grub_errno,
		    grub_errmsg);

  if (err != GRUB_ERR_NONE)
    return;

  DO_TEST (hi_signed_2nd, 1);
  DO_TEST (hi_signed, 1);
  DO_TEST (hi_double, 1);
  DO_TEST (hi_double_extended, 1);

  /*
   * Check certificate removal. They're added to the _top_ of the list and
   * removed by position in the list. Current the list looks like [#2, #1].
   *
   * First test removing the second certificate in the list, which is
   * certificate #1, giving us just [#2].
   */
  cmd_distrust = grub_command_find ("distrust_certificate");
  if (!cmd_distrust)
    {
      grub_test_assert (0, "can't find command `%s'", "distrust_certificate");
      return;
    }

  err = (cmd_distrust->func) (cmd_distrust, 1, distrust2_args);
  grub_test_assert (err == GRUB_ERR_NONE,
		    "distrusting certificate 1 failed: %d: %s", grub_errno,
		    grub_errmsg);
  DO_TEST (hi_signed_2nd, 1);
  DO_TEST (hi_signed, 0);
  DO_TEST (hi_double, 1);

  /*
   * Now reload certificate #1. This will make the list look like [#1, #2]
   */
  err = (cmd_trust->func) (cmd_trust, 1, trust_args);

  grub_test_assert (err == GRUB_ERR_NONE,
		    "reloading certificate 1 failed: %d: %s", grub_errno,
		    grub_errmsg);
  DO_TEST (hi_signed, 1);

  /* Remove the first certificate in the list, giving us just [#2] */
  err = (cmd_distrust->func) (cmd_distrust, 1, distrust_args);
  grub_test_assert (err == GRUB_ERR_NONE,
		    "distrusting certificate 1 (first time) failed: %d: %s",
		    grub_errno, grub_errmsg);
  DO_TEST (hi_signed_2nd, 1);
  DO_TEST (hi_signed, 0);

  /*
   * Remove the first certificate again, giving an empty list.
   *
   * verify_appended should fail if there are no certificates to verify against.
   */
  err = (cmd_distrust->func) (cmd_distrust, 1, distrust_args);
  grub_test_assert (err == GRUB_ERR_NONE,
		    "distrusting certificate 1 (second time) failed: %d: %s",
		    grub_errno, grub_errmsg);
  DO_TEST (hi_signed_2nd, 0);
  DO_TEST (hi_double, 0);

  /*
   * Lastly, check a certificate that uses printableString rather than
   * utf8String loads properly, and that a certificate with an appropriate
   * extended key usage loads.
   */
  err = (cmd_trust->func) (cmd_trust, 1, trust_args_printable);
  grub_test_assert (err == GRUB_ERR_NONE,
		    "trusting printable certificate failed: %d: %s",
		    grub_errno, grub_errmsg);

  err = (cmd_trust->func) (cmd_trust, 1, trust_args_eku);
  grub_test_assert (err == GRUB_ERR_NONE,
		    "trusting certificate with extended key usage failed: %d: %s",
		    grub_errno, grub_errmsg);

  grub_procfs_unregister (&certificate_der_entry);
  grub_procfs_unregister (&certificate2_der_entry);
  grub_procfs_unregister (&certificate_printable_der_entry);
  grub_procfs_unregister (&certificate_eku_der_entry);
}

GRUB_FUNCTIONAL_TEST (appended_signature_test, appended_signature_test);
