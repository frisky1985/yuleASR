"""yuleASR-Configurator CLI entry point."""

from __future__ import annotations

import logging
import sys
from pathlib import Path
from typing import List, Optional

import typer
from rich.console import Console
from rich.table import Table

app = typer.Typer(
    name="yuleasr",
    help="yuleASR-Configurator: Unified AUTOSAR BSW configuration tool",
    add_completion=False,
)
console = Console()

REPO_ROOT = Path(__file__).resolve().parents[3]
DEFAULT_CONFIG = REPO_ROOT / "config" / "bsw_config.json"
DEFAULT_TEMPLATES = REPO_ROOT / "config" / "templates"
DEFAULT_OUTPUT = REPO_ROOT / "config" / "generated"
DEFAULT_SCHEMA = REPO_ROOT / "config"


def _setup_logging(verbose: bool) -> None:
    level = logging.DEBUG if verbose else logging.INFO
    logging.basicConfig(
        level=level,
        format="%(levelname)-8s %(name)s: %(message)s",
        stream=sys.stderr,
    )


@app.command()
def generate(
    config: Path = typer.Option(DEFAULT_CONFIG, "--config", "-c", help="Config file (YAML/JSON)"),
    output: Path = typer.Option(DEFAULT_OUTPUT, "--output", "-o", help="Output directory"),
    templates: Path = typer.Option(DEFAULT_TEMPLATES, "--templates", "-t", help="Templates directory"),
    module: Optional[List[str]] = typer.Option(None, "--module", "-m", help="Generate only these modules"),
    no_validate: bool = typer.Option(False, "--no-validate", help="Skip schema validation"),
    dry_run: bool = typer.Option(False, "--dry-run", help="Check drift without writing"),
    verbose: bool = typer.Option(False, "--verbose", "-v"),
) -> None:
    """Generate BSW configuration C files from YAML/JSON config."""
    _setup_logging(verbose)

    from yuleasr.core.code_generator import CodeGenerator

    gen = CodeGenerator(
        config_path=config,
        templates_dir=templates,
        output_dir=output,
        schema_dir=DEFAULT_SCHEMA,
    )
    report = gen.run(
        module_filter=module,
        write=not dry_run,
        validate=not no_validate,
    )

    if report.validation:
        console.print(f"\n[bold]Validation:[/bold] {report.validation.error_count} error(s), {report.validation.warning_count} warning(s)")

    if dry_run:
        console.print(f"\n[bold]Drift check:[/bold] {len(report.drifts)} drift(s) found")
        for d in report.drifts:
            console.print(f"  [yellow]{d}[/yellow]")
    else:
        console.print(f"\n[bold]Generated:[/bold] {len(report.written)} file(s)")
        for w in report.written:
            console.print(f"  [green]{w}[/green]")

    if report.errors:
        console.print(f"\n[bold red]Errors:[/bold red]")
        for e in report.errors:
            console.print(f"  [red]{e}[/red]")

    raise typer.Exit(0 if report.ok else 1)


@app.command()
def validate(
    config: Path = typer.Option(DEFAULT_CONFIG, "--config", "-c", help="Config file"),
    schema_dir: Path = typer.Option(DEFAULT_SCHEMA, "--schema-dir", "-s", help="Schema directory"),
    strict: bool = typer.Option(False, "--strict", help="Treat warnings as errors"),
    verbose: bool = typer.Option(False, "--verbose", "-v"),
) -> None:
    """Validate configuration against JSON Schema."""
    _setup_logging(verbose)

    from yuleasr.core.config_loader import ConfigLoadError, load_config
    from yuleasr.core.schema_engine import SchemaRegistry, fallback_validate

    try:
        cfg = load_config(config)
    except ConfigLoadError as e:
        console.print(f"[red]ERROR:[/red] {e}")
        raise typer.Exit(2)

    if schema_dir.is_dir():
        registry = SchemaRegistry(schema_dir)
        result = registry.validate_all(cfg)
    else:
        result = fallback_validate(cfg)

    console.print(f"[bold]Validation:[/bold] {result.error_count} error(s), {result.warning_count} warning(s)")
    for e in result.errors:
        color = "red" if e.severity == "error" else "yellow"
        console.print(f"  [{color}]{e}[/{color}]")

    ok = result.ok and (not strict or result.warning_count == 0)
    raise typer.Exit(0 if ok else 1)


@app.command()
def init(
    target: str = typer.Option("s32k312", "--target", "-t", help="MCU target name"),
    output: Path = typer.Option(Path("project"), "--output", "-o", help="Output directory"),
    verbose: bool = typer.Option(False, "--verbose", "-v"),
) -> None:
    """Create a new project configuration scaffold."""
    _setup_logging(verbose)

    from yuleasr.core.config_loader import dump_yaml

    output.mkdir(parents=True, exist_ok=True)

    scaffold = {
        "version": "1.0",
        "project": {
            "name": "yuleASR Project",
            "target": target,
        },
        "target": {"name": target},
        "modules": {},
    }

    config_file = output / "project.yaml"
    dump_yaml(scaffold, config_file)
    console.print(f"[green]Created[/green] {config_file}")


@app.command()
def list_modules() -> None:
    """List all supported BSW modules."""
    table = Table(title="Supported Modules")
    table.add_column("Layer", style="cyan")
    table.add_column("Module", style="green")

    modules = {
        "MCAL": ["Mcu", "Port", "Dio", "Can", "Spi", "Adc", "Pwm", "Gpt", "Wdg", "Icu", "Lin"],
        "ECUAL": ["CanIf", "CanTp", "IoHwAb", "EthIf", "MemIf", "Fee", "Ea", "FrIf", "LinIf"],
        "Services": ["Com", "PduR", "NvM", "Dcm", "Dem"],
        "OS": ["Os"],
        "Integration": ["BswM", "EcuM"],
    }
    for layer, mods in modules.items():
        for mod in mods:
            table.add_row(layer, mod)

    console.print(table)


@app.command()
def init_order(
    config: Path = typer.Option(DEFAULT_CONFIG, "--config", "-c", help="Config file"),
    verbose: bool = typer.Option(False, "--verbose", "-v"),
) -> None:
    """Show module initialization order based on dependencies."""
    _setup_logging(verbose)

    from yuleasr.core.config_loader import ConfigLoadError, load_config
    from yuleasr.core.resolver import ReferenceResolver

    try:
        cfg = load_config(config)
    except ConfigLoadError as e:
        console.print(f"[red]ERROR:[/red] {e}")
        raise typer.Exit(2)

    resolver = ReferenceResolver(cfg)
    report = resolver.get_init_order_report()
    console.print(report)

    result = resolver.resolve()
    raise typer.Exit(0 if result.ok else 1)


if __name__ == "__main__":
    app()
