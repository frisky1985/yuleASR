"""Shared pytest fixtures for the yuleASR Configurator test suite.

The package is used from a source checkout (no ``pip install -e``), so the
package root is prepended to ``sys.path`` before any ``yuleasr`` import.
"""

import sys
from pathlib import Path

import pytest

PKG_ROOT = Path(__file__).resolve().parents[1]
if str(PKG_ROOT) not in sys.path:
    sys.path.insert(0, str(PKG_ROOT))


@pytest.fixture(scope="session")
def api():
    """Import the FastAPI application module once per test session."""
    from yuleasr.web.api import main

    return main


@pytest.fixture()
def client(api):
    """Starlette test client bound to the real application object."""
    from fastapi.testclient import TestClient

    return TestClient(api.app)


@pytest.fixture()
def sandbox_repo(api, tmp_path, monkeypatch):
    """Point the API's ``REPO_ROOT`` at a temporary directory.

    ``POST /api/project/create`` and ``GET /api/projects`` resolve their
    paths from the module-level ``REPO_ROOT``; redirecting it keeps every
    write inside ``tmp_path`` instead of the source checkout.
    """
    monkeypatch.setattr(api, "REPO_ROOT", tmp_path)
    return tmp_path
