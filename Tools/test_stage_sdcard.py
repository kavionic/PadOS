# This file is part of PadOS.
# Copyright (c) 2026 Kurt Skauen
# SPDX-License-Identifier: Apache-2.0

"""Host tests for SD-card staging. No device connection is opened."""

import json
import os
from pathlib import Path
import tempfile
import unittest
from unittest import mock

from stage_sdcard import stage_files


class SDCardStagingTests(unittest.TestCase):
    def setUp(self):
        self.m_Temporary = tempfile.TemporaryDirectory(prefix="pados-sdcard-test-")
        self.addCleanup(self.m_Temporary.cleanup)
        self.m_Root = Path(self.m_Temporary.name)
        self.m_Generic = self.m_Root / "generic"
        self.m_Project = self.m_Root / "project"
        self.m_Output = self.m_Root / "SDCard"
        self.m_State = self.m_Root / "state/files.json"
        self.m_Generic.mkdir()
        self.m_Project.mkdir()
        self.m_Sources = [(self.m_Generic, "Rainbow3D"), (self.m_Project, ".")]

    def write(self, path, text):
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(text, encoding="utf-8")

    def stage(self, verify=False):
        stage_files(self.m_Output, self.m_State, self.m_Sources, verify=verify)

    def test_project_override_and_fallback_after_source_removal(self):
        relative = "Rainbow3D/System/default.xml"
        self.write(self.m_Generic / "System/default.xml", "generic")
        self.write(self.m_Project / relative, "project")
        self.stage()
        self.assertEqual((self.m_Output / relative).read_text(), "project")
        modified = (self.m_Output / relative).stat().st_mtime_ns
        self.stage()
        self.assertEqual((self.m_Output / relative).stat().st_mtime_ns, modified)
        (self.m_Project / relative).unlink()
        self.stage()
        self.assertEqual((self.m_Output / relative).read_text(), "generic")

    def test_unchanged_files_reuse_hashes_and_leave_state_unchanged(self):
        self.write(self.m_Project / "asset.txt", "first")
        self.stage()
        state_modified = self.m_State.stat().st_mtime_ns
        with mock.patch("stage_sdcard.file_hash", side_effect=AssertionError("Unchanged file was read")):
            self.stage()
        self.assertEqual(self.m_State.stat().st_mtime_ns, state_modified)

    def test_same_size_source_and_destination_changes_are_detected(self):
        source = self.m_Project / "asset.txt"
        target = self.m_Output / "asset.txt"
        self.write(source, "first")
        self.stage()
        modified = source.stat().st_mtime_ns + 1_000_000_000
        self.write(source, "other")
        os.utime(source, ns=(modified, modified))
        self.stage()
        self.assertEqual(target.read_text(), "other")
        self.write(target, "local")
        os.utime(target, ns=(modified + 1_000_000_000, modified + 1_000_000_000))
        self.stage()
        self.assertEqual(target.read_text(), "other")

    def test_override_change_with_equal_size_and_timestamp_is_detected(self):
        generic = self.m_Generic / "asset.txt"
        project = self.m_Project / "Rainbow3D/asset.txt"
        self.write(generic, "first")
        self.write(project, "other")
        modified = generic.stat().st_mtime_ns
        os.utime(project, ns=(modified, modified))
        self.stage()
        project.unlink()
        self.stage()
        self.assertEqual((self.m_Output / "Rainbow3D/asset.txt").read_text(), "first")

    def test_legacy_state_is_upgraded_without_copying_unchanged_files(self):
        self.write(self.m_Project / "asset.txt", "first")
        self.stage()
        records = json.loads(self.m_State.read_text())
        self.m_State.write_text(json.dumps({name: record["sha256"] for name, record in records.items()}))
        with mock.patch("stage_sdcard.shutil.copy2", side_effect=AssertionError("Unchanged file was copied")):
            self.stage()
        self.assertIsInstance(json.loads(self.m_State.read_text())["asset.txt"], dict)

    def test_verification_detects_changes_with_unchanged_size_and_timestamp(self):
        source = self.m_Project / "asset.txt"
        self.write(source, "first")
        self.stage()
        modified = source.stat().st_mtime_ns
        self.write(source, "other")
        os.utime(source, ns=(modified, modified))
        self.stage(verify=True)
        self.assertEqual((self.m_Output / "asset.txt").read_text(), "other")

    def test_added_removed_and_missing_files_preserve_unmanaged_files(self):
        self.write(self.m_Project / "obsolete/file.txt", "old")
        self.write(self.m_Project / "restore.txt", "restore")
        self.stage()
        self.write(self.m_Output / "downloaded.txt", "keep")
        (self.m_Output / "restore.txt").unlink()
        (self.m_Project / "obsolete/file.txt").unlink()
        self.write(self.m_Project / "new/file.txt", "new")
        self.stage()
        self.assertFalse((self.m_Output / "obsolete").exists())
        self.assertEqual((self.m_Output / "new/file.txt").read_text(), "new")
        self.assertEqual((self.m_Output / "restore.txt").read_text(), "restore")
        self.assertEqual((self.m_Output / "downloaded.txt").read_text(), "keep")

    def test_control_edits_are_preserved_and_conflicts_do_not_modify_output(self):
        self.write(self.m_Project / ".syncignore", "first\n")
        self.write(self.m_Project / ".deleterules", "original\n")
        self.stage()
        self.write(self.m_Output / ".syncignore", "local\n")
        self.write(self.m_Project / ".deleterules", "updated\n")
        self.stage()
        self.assertEqual((self.m_Output / ".syncignore").read_text(), "local\n")
        self.assertEqual((self.m_Output / ".deleterules").read_text(), "updated\n")
        previous_state = self.m_State.read_bytes()
        self.write(self.m_Project / ".syncignore", "source\n")
        with self.assertRaisesRegex(ValueError, "Both source and staged"):
            self.stage()
        self.assertEqual((self.m_Output / ".syncignore").read_text(), "local\n")
        self.assertEqual(self.m_State.read_bytes(), previous_state)

    def test_build_files_excluded_and_nested_sync_metadata_retained(self):
        self.write(self.m_Project / "CMakeLists.txt", "build-only")
        self.write(self.m_Project / "Rainbow3D/.gitignore", "git-only")
        self.write(self.m_Project / ".syncignore", "*.tmp\n")
        self.write(self.m_Project / "Rainbow3D/System/UNIX/.syncignore", "keyboard-data.json\n")
        self.write(self.m_Project / "Rainbow3D/System/UNIX/keyboard-data.json", "{}")
        self.stage()
        self.assertFalse((self.m_Output / "CMakeLists.txt").exists())
        self.assertFalse((self.m_Output / "Rainbow3D/.gitignore").exists())
        self.assertTrue((self.m_Output / ".syncignore").is_file())
        self.assertTrue((self.m_Output / "Rainbow3D/System/UNIX/keyboard-data.json").is_file())
        self.assertTrue((self.m_Output / "Rainbow3D/System/UNIX/.syncignore").is_file())

    def test_invalid_old_state_cannot_delete_outside_output(self):
        outside = self.m_Root / "keep.txt"
        self.write(outside, "keep")
        self.write(self.m_State, json.dumps({"../keep.txt": "old"}))
        with self.assertRaisesRegex(ValueError, "Invalid staged path"):
            self.stage()
        self.assertEqual(outside.read_text(), "keep")

    def test_destination_prefix_must_stay_inside_output(self):
        self.write(self.m_Generic / "asset.txt", "keep")
        with self.assertRaisesRegex(ValueError, "Invalid destination prefix"):
            stage_files(self.m_Output, self.m_State, [(self.m_Generic, "../escape")])
        self.assertFalse((self.m_Root / "escape").exists())

    def test_case_collision_and_file_directory_collision_are_rejected(self):
        self.write(self.m_Generic / "System/value.txt", "generic")
        self.write(self.m_Project / "Rainbow3D/System/Value.txt", "project")
        with self.assertRaisesRegex(ValueError, "differ only in case"):
            self.stage()
        (self.m_Project / "Rainbow3D/System/Value.txt").unlink()
        self.write(self.m_Project / "Rainbow3D/System/value.txt/child.txt", "conflict")
        with self.assertRaisesRegex(ValueError, "both a file and directory"):
            self.stage()


if __name__ == "__main__":
    unittest.main()
