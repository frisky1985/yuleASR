"""Smoke tests for the Configurator HTTP API.

Covers the project-lifecycle paths the Vue front-end drives on startup:

* save / load a ``.yuleasr`` project file
* create a project from wizard data and see it in the project list
* import an ARXML configuration, export it back, and re-parse it
* delete a project file

Every test writes under ``tmp_path`` (or a redirected ``REPO_ROOT``) so the
source checkout is never modified.
"""

import json

import pytest

# ARXML fixture: one ECUC-VALUE container with a numeric, a boolean and a
# string parameter, using the AUTOSAR r4.0 namespace the parser expects.
ARXML_ADC = """<?xml version="1.0" encoding="UTF-8"?>
<AUTOSAR xmlns="http://autosar.org/schema/r4.0">
  <ECUC-VALUES>
    <ECUC-VALUE>
      <CONTAINER>
        <SHORT-NAME>Adc</SHORT-NAME>
        <ECUC-NUMERICAL-PARAM-VALUE>
          <DEFINITION-REF DEST="ECUC-INTEGER-PARAM-DEF">/AUTOSAR/EcucDefs/Adc/AdcClockFrequency</DEFINITION-REF>
          <VALUE>80000000</VALUE>
        </ECUC-NUMERICAL-PARAM-VALUE>
        <ECUC-BOOLEAN-PARAM-VALUE>
          <DEFINITION-REF DEST="ECUC-BOOLEAN-PARAM-DEF">/AUTOSAR/EcucDefs/Adc/AdcEnable</DEFINITION-REF>
          <VALUE>TRUE</VALUE>
        </ECUC-BOOLEAN-PARAM-VALUE>
        <ECUC-STRING-PARAM-VALUE>
          <DEFINITION-REF DEST="ECUC-TEXTUAL-PARAM-DEF">/AUTOSAR/EcucDefs/Adc/AdcHwUnit</DEFINITION-REF>
          <VALUE>ADC0</VALUE>
        </ECUC-STRING-PARAM-VALUE>
      </CONTAINER>
    </ECUC-VALUE>
  </ECUC-VALUES>
</AUTOSAR>
"""


def _project_payload(name="Smoke Project", ecu_id="ecu-smoke01"):
    """Minimal but complete ``.yuleasr`` payload matching ProjectFile.to_dict()."""
    return {
        "version": "1.0",
        "project": {
            "name": name,
            "description": "created by api smoke test",
            "created": "2026-01-01T00:00:00Z",
            "modified": "2026-01-01T00:00:00Z",
        },
        "ecus": [
            {
                "id": ecu_id,
                "name": "Smoke_ECU",
                "chip": {"vendor": "NXP", "series": "S32K", "model": "S32K144", "cores": 1},
                "config_path": "bsw_config.yaml",
                "description": "primary ecu",
                "modules": {"Adc": {"name": "Adc", "enabled": True}},
            }
        ],
        "ui_state": {
            "sidebar_visible": True,
            "sidebar_width": 240,
            "active_tab": "Adc",
            "open_tabs": ["Adc"],
            "expanded_layers": ["mcal"],
        },
        "recent_files": [],
    }


# ---------------------------------------------------------------------------
# Liveness
# ---------------------------------------------------------------------------


def test_health(client):
    response = client.get("/health")
    assert response.status_code == 200


# ---------------------------------------------------------------------------
# Path 1 - save / load a project file
# ---------------------------------------------------------------------------


def test_save_project_writes_file_and_returns_path(client, tmp_path):
    project_file = tmp_path / "sandbox" / "smoke.yuleasr"

    response = client.post(
        "/api/project", params={"project_path": str(project_file)}, json=_project_payload()
    )

    assert response.status_code == 200, response.text
    assert response.json()["success"] is True
    assert response.json()["path"] == str(project_file)
    assert project_file.is_file(), "save endpoint reported success but wrote nothing"


def test_save_project_roundtrips_through_load(client, tmp_path):
    project_file = tmp_path / "smoke.yuleasr"
    client.post("/api/project", params={"project_path": str(project_file)}, json=_project_payload())

    response = client.get("/api/project", params={"project_path": str(project_file)})

    assert response.status_code == 200, response.text
    body = response.json()
    assert body["project"]["name"] == "Smoke Project"
    assert body["project"]["description"] == "created by api smoke test"
    assert len(body["ecus"]) == 1
    assert body["ecus"][0]["id"] == "ecu-smoke01"
    assert body["ecus"][0]["chip"]["model"] == "S32K144"
    assert body["ecus"][0]["modules"]["Adc"]["enabled"] is True
    assert body["ui_state"]["active_tab"] == "Adc"


def test_saved_file_is_valid_json_with_project_structure(client, tmp_path):
    project_file = tmp_path / "smoke.yuleasr"
    client.post("/api/project", params={"project_path": str(project_file)}, json=_project_payload())

    on_disk = json.loads(project_file.read_text(encoding="utf-8"))

    assert on_disk["project"]["name"] == "Smoke Project"
    assert on_disk["ecus"][0]["name"] == "Smoke_ECU"


def test_load_missing_project_creates_empty_file(client, tmp_path):
    project_file = tmp_path / "fresh.yuleasr"

    response = client.get("/api/project", params={"project_path": str(project_file)})

    assert response.status_code == 200, response.text
    assert project_file.is_file()
    assert response.json()["ecus"] == []


# ---------------------------------------------------------------------------
# Path 2 - create from wizard data, list, then delete
# ---------------------------------------------------------------------------


def test_create_project_lays_out_project_file_and_config(client, sandbox_repo):
    response = client.post(
        "/api/project/create",
        json={
            "name": "WizardProject",
            "description": "wizard smoke",
            "author": "ci",
            "mcal_modules": ["Adc", "Dio"],
            "comm_modules": ["Can"],
        },
    )

    assert response.status_code == 200, response.text
    body = response.json()
    assert body["success"] is True
    assert body["module_count"] == 3
    assert body["ecu_id"]

    project_file = sandbox_repo / "projects" / "WizardProject" / ".yuleasr"
    config_file = sandbox_repo / "projects" / "WizardProject" / "bsw_config.yaml"
    assert project_file.is_file()
    assert config_file.is_file()

    saved = json.loads(project_file.read_text(encoding="utf-8"))
    assert saved["project"]["name"] == "WizardProject"
    assert saved["ecus"][0]["name"] == "WizardProject_ECU"

    config_text = config_file.read_text(encoding="utf-8")
    assert "WizardProject" in config_text
    assert "Adc" in config_text


def test_project_list_includes_created_project(client, sandbox_repo):
    client.post("/api/project/create", json={"name": "ListedProject", "mcal_modules": ["Adc"]})

    response = client.get("/api/projects")

    assert response.status_code == 200, response.text
    names = [p["name"] for p in response.json()["projects"]]
    assert "ListedProject" in names
    listed = next(p for p in response.json()["projects"] if p["name"] == "ListedProject")
    assert listed["ecu_count"] == 1


def test_delete_project_removes_file(client, tmp_path):
    project_file = tmp_path / "doomed.yuleasr"
    client.post("/api/project", params={"project_path": str(project_file)}, json=_project_payload())
    assert project_file.is_file()

    response = client.delete("/api/project", params={"project_path": str(project_file)})

    assert response.status_code == 200, response.text
    assert response.json()["success"] is True
    assert not project_file.exists()


def test_delete_project_requires_a_path(client):
    response = client.delete("/api/project")
    assert response.status_code == 400


# ---------------------------------------------------------------------------
# Path 3 - ARXML import / export
# ---------------------------------------------------------------------------


def test_import_arxml_extracts_module_parameters(client):
    response = client.post("/api/arxml/import", json={"content": ARXML_ADC})

    assert response.status_code == 200, response.text
    body = response.json()
    assert body["success"] is True
    assert body["modules"] == ["Adc"]
    adc = body["config"]["modules"]["Adc"]
    assert adc["adcclockfrequency"] == 80000000
    assert adc["adcenable"] is True
    assert adc["adchwunit"] == "ADC0"


def test_import_arxml_rejects_malformed_content(client):
    response = client.post("/api/arxml/import", json={"content": "<AUTOSAR><unclosed>"})
    assert response.status_code == 400


def test_export_arxml_roundtrips_imported_config(client):
    imported = client.post("/api/arxml/import", json={"content": ARXML_ADC}).json()

    exported = client.post("/api/arxml/export", json={"config": imported["config"]})

    assert exported.status_code == 200, exported.text
    assert exported.json()["module_count"] == 1
    arxml_text = exported.json()["arxml"]
    assert "Adc" in arxml_text

    reparsed = client.post("/api/arxml/import", json={"content": arxml_text})
    assert reparsed.status_code == 200, reparsed.text
    assert reparsed.json()["modules"] == ["Adc"]
    assert reparsed.json()["config"]["modules"]["Adc"]["adcclockfrequency"] == 80000000
