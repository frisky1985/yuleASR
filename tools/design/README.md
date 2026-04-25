# DDS Design-Time Support Tools

Provides design validation and analysis tools for DDS-based automotive systems.

## Components

### 1. Design Rule Checker (`design_rule_checker.py`)
Validates DDS configurations against automotive design rules.

```python
from design_rule_checker import DesignRuleChecker

checker = DesignRuleChecker()
violations = checker.check_file("config.yaml")
checker.print_report()
```

**Checked Rules:**
- Naming conventions (PascalCase for topics/types)
- Safety-critical QoS requirements
- Deadline configuration
- E2E protection
- Resource limits
- Security policies
- AUTOSAR compliance

### 2. Data Flow Analyzer (`data_flow_analyzer.py`)
Analyzes data flows and detects cycles.

```python
from data_flow_analyzer import DataFlowAnalyzer

analyzer = DataFlowAnalyzer()
analyzer.build_from_config(config)

# Find cycles
cycles = analyzer.find_cycles()

# Get data flow report
report = analyzer.generate_report()

# Export to Graphviz DOT
dot = analyzer.export_to_dot()
```

### 3. Timing Calculator (`timing_calculator.py`)
Calculates end-to-end latency and validates timing constraints.

```python
from timing_calculator import TimingCalculator, ComponentTiming, TimingConstraint

calc = TimingCalculator()

# Add component timing
calc.add_component_timing(ComponentTiming(
    name="Controller",
    worst_case_execution_time_ms=2.0,
    best_case_execution_time_ms=0.5,
    period_ms=10.0
))

# Add deadline constraint
calc.add_constraint("deadline", TimingConstraint(
    type=TimingConstraintType.DEADLINE,
    value_ms=5.0
))

# Calculate end-to-end latency
e2e = calc.calculate_end_to_end_latency(
    path_components=["Sensor", "Controller", "Actuator"],
    network_path=["eth0"]
)

print(f"WCET: {e2e.worst_case_latency_ms} ms")
print(f"Deadline met: {e2e.deadline_met}")
```

## Design Rules

### Naming Conventions
- **Topics**: PascalCase (e.g., `VehicleSpeed`)
- **Types**: PascalCase (e.g., `SpeedData`)
- **QoS Profiles**: camelCase or snake_case (e.g., `reliableLowLatency`)

### Safety Requirements
- Safety-critical topics must use RELIABLE reliability
- Control topics should have deadline configured
- E2E protection recommended for ASIL-D systems

### Performance Guidelines
- History depth should not exceed 100 samples
- Resource limits should be configured for bounded memory
- Consider transport priority for critical data

### Security Requirements
- Security should be enabled for production
- Authentication certificates must be configured when security is enabled
- Encryption recommended for sensitive data

## Example Usage

```bash
# Run design rule check
python -m design.design_rule_checker config.yaml

# Analyze data flows
python -m design.data_flow_analyzer config.yaml --export-graph flow.dot

# Calculate timing
python -m design.timing_calculator config.yaml --report timing.json
```