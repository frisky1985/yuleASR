"""Golden file testing framework for yuleASR-Configurator.

Compares generated output against committed golden files to detect regressions.
"""

from __future__ import annotations

import difflib
import logging
from dataclasses import dataclass, field
from pathlib import Path
from typing import List, Tuple

LOG = logging.getLogger(__name__)


@dataclass
class GoldenTestResult:
    """Result of golden file comparison."""
    golden_file: Path
    generated_file: Path
    match: bool
    diff_lines: List[str] = field(default_factory=list)

    @property
    def filename(self) -> str:
        return self.golden_file.name


class GoldenFileTester:
    """Tests generated output against golden files."""

    def __init__(
        self,
        golden_dir: Path,
        generated_dir: Path,
    ):
        self.golden_dir = golden_dir
        self.generated_dir = generated_dir

    def run_all(self) -> Tuple[List[GoldenTestResult], List[GoldenTestResult]]:
        """Run golden file tests for all files.

        Returns (matches, mismatches).
        """
        matches = []
        mismatches = []

        golden_files = sorted(self.golden_dir.glob("*.h"))
        LOG.info(f"Found {len(golden_files)} golden files")

        for golden_file in golden_files:
            generated_file = self.generated_dir / golden_file.name

            if not generated_file.exists():
                LOG.warning(f"Generated file missing: {generated_file.name}")
                mismatches.append(GoldenTestResult(
                    golden_file=golden_file,
                    generated_file=generated_file,
                    match=False,
                    diff_lines=[f"MISSING: {generated_file.name}"],
                ))
                continue

            result = self.compare_files(golden_file, generated_file)
            if result.match:
                matches.append(result)
            else:
                mismatches.append(result)

        return matches, mismatches

    def compare_files(self, golden_file: Path, generated_file: Path) -> GoldenTestResult:
        """Compare a single golden file against generated output."""
        golden_content = golden_file.read_text(encoding="utf-8")
        generated_content = generated_file.read_text(encoding="utf-8")

        if golden_content == generated_content:
            return GoldenTestResult(
                golden_file=golden_file,
                generated_file=generated_file,
                match=True,
            )

        # Generate diff
        diff = list(difflib.unified_diff(
            golden_content.splitlines(keepends=True),
            generated_content.splitlines(keepends=True),
            fromfile=f"golden/{golden_file.name}",
            tofile=f"generated/{generated_file.name}",
            lineterm="",
        ))

        return GoldenTestResult(
            golden_file=golden_file,
            generated_file=generated_file,
            match=False,
            diff_lines=diff,
        )

    def update_golden(self, filename: str) -> bool:
        """Update golden file with generated version.

        Returns True if updated, False if generated file doesn't exist.
        """
        golden_file = self.golden_dir / filename
        generated_file = self.generated_dir / filename

        if not generated_file.exists():
            LOG.error(f"Cannot update: {generated_file.name} does not exist")
            return False

        generated_file.rename(golden_file)
        LOG.info(f"Updated golden file: {filename}")
        return True

    def update_all(self, mismatches: List[GoldenTestResult]) -> int:
        """Update all mismatched golden files.

        Returns number of files updated.
        """
        updated = 0
        for result in mismatches:
            if result.generated_file.exists():
                result.generated_file.rename(result.golden_file)
                LOG.info(f"Updated: {result.filename}")
                updated += 1
        return updated

    def print_report(
        self,
        matches: List[GoldenTestResult],
        mismatches: List[GoldenTestResult],
        show_diff: bool = False,
    ) -> None:
        """Print golden file test report."""
        total = len(matches) + len(mismatches)

        print("\n" + "=" * 70)
        print("Golden File Test Report")
        print("=" * 70)
        print(f"Total files:  {total}")
        print(f"Matches:      {len(matches)} ✓")
        print(f"Mismatches:   {len(mismatches)} ✗")
        print("=" * 70)

        if mismatches:
            print("\nMismatched files:")
            for result in mismatches:
                print(f"  ✗ {result.filename}")
                if show_diff and result.diff_lines:
                    print("\n" + "-" * 70)
                    print("".join(result.diff_lines[:50]))  # Limit diff output
                    if len(result.diff_lines) > 50:
                        print(f"... ({len(result.diff_lines) - 50} more lines)")
                    print("-" * 70)

        if matches and not mismatches:
            print("\n✓ All golden files match!")

        print("=" * 70)


def main():
    """CLI entry point for golden file testing."""
    import argparse

    parser = argparse.ArgumentParser(description="Golden file testing for yuleASR-Configurator")
    parser.add_argument("--golden-dir", type=Path, default=Path("config/generated"),
                        help="Directory containing golden files")
    parser.add_argument("--generated-dir", type=Path, default=Path("/tmp/yuleasr-golden-test"),
                        help="Directory containing generated files")
    parser.add_argument("--update", action="store_true",
                        help="Update golden files with generated versions")
    parser.add_argument("--show-diff", action="store_true",
                        help="Show diff for mismatched files")
    parser.add_argument("--verbose", "-v", action="store_true",
                        help="Verbose output")

    args = parser.parse_args()

    logging.basicConfig(
        level=logging.DEBUG if args.verbose else logging.INFO,
        format="%(levelname)-8s %(message)s",
    )

    tester = GoldenFileTester(args.golden_dir, args.generated_dir)
    matches, mismatches = tester.run_all()

    if args.update and mismatches:
        print(f"\nUpdating {len(mismatches)} golden files...")
        updated = tester.update_all(mismatches)
        print(f"Updated {updated} files")
        return

    tester.print_report(matches, mismatches, show_diff=args.show_diff)

    # Exit with error if mismatches found
    if mismatches:
        exit(1)


if __name__ == "__main__":
    main()
