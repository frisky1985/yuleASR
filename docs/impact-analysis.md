# yuleASR — Impact Analysis Document

> **Version**: 1.0 | **Last Updated**: 2026-07-15
> **Project**: yuleASR AUTOSAR BSW Platform (S32K312)

## Scope

This document records the impact analysis for changes to the yuleASR software requirements, architecture, and implementation.

## Change History

| Change ID | Date | Description | Requester | Risk Level |
|:----------|:-----|:------------|:----------|:-----------|
| CHG-001 | 2026-01-15 | Initial BSW platform baseline | Architecture Team | Low |
| CHG-002 | 2026-03-01 | Added Crypto HSM integration | Security Team | Medium |
| CHG-003 | 2026-04-10 | Added secure boot support | Safety Team | High |
| CHG-004 | 2026-05-20 | Micro DDS middleware integration | Communication Team | Medium |

## Impact Analysis Template

### Change: [CHG-NNN] — [Title]

**Impact on Schedule**:
- Estimated implementation effort: [X] person-days
- Testing effort: [Y] person-days
- Integration effort: [Z] person-days

**Impact on Resources**:
- Additional team members required: [Yes/No]
- Additional tooling/hardware: [Details]
- Training requirements: [Details]

**Impact on Architecture**:
- Components affected: [List]
- Interfaces affected: [List]
- Backward compatibility: [Yes/No]

**Impact on Requirements**:
- Requirements added: [Count]
- Requirements modified: [Count]
- Requirements deprecated: [Count]

**Risk Assessment**:
- Overall risk: [Low/Medium/High]
- Mitigation measures: [Description]
- Contingency plan: [Description]

## Current Impact Analysis

### CHG-004: Micro DDS Middleware Integration

**Impact on Schedule**:
- Estimated implementation effort: 15 person-days
- Testing effort: 5 person-days
- Integration effort: 3 person-days

**Impact on Resources**:
- Additional team members required: No
- Additional tooling/hardware: No
- Training requirements: DDS concepts training (2 days)

**Impact on Architecture**:
- Components affected: RTE, Communication Stack, SoAd
- Interfaces affected: SomeIp-to-DDS bridging layer
- Backward compatibility: Yes (DDS is additive)

**Impact on Requirements**:
- Requirements added: 2 (SWR-008.1-01, SWR-008.1-02)
- Requirements modified: 0
- Requirements deprecated: 0

**Risk Assessment**:
- Overall risk: Medium
- Mitigation measures: Incremental integration with existing SomeIp stack
