#!/usr/bin/env python3
"""
Timing Calculator for DDS Systems
Calculates end-to-end latency, analyzes timing constraints, and validates deadlines
"""

import math
from typing import List, Dict, Optional, Tuple, Any
from dataclasses import dataclass, field
from enum import Enum


class TimingConstraintType(Enum):
    """Types of timing constraints"""
    DEADLINE = "deadline"
    LATENCY = "latency"
    JITTER = "jitter"
    PERIOD = "period"
    OFFSET = "offset"


@dataclass
class TimingConstraint:
    """Timing constraint definition"""
    type: TimingConstraintType
    value_ms: float
    is_hard: bool = True  # Hard vs soft real-time
    confidence: float = 0.95  # Statistical confidence level


@dataclass
class ComponentTiming:
    """Timing characteristics of a component"""
    name: str
    worst_case_execution_time_ms: float
    best_case_execution_time_ms: float
    average_execution_time_ms: float
    period_ms: Optional[float] = None
    priority: int = 0
    core_affinity: Optional[int] = None


@dataclass
class NetworkTiming:
    """Network timing characteristics"""
    protocol: str
    bandwidth_mbps: float
    propagation_delay_ms: float
    transmission_delay_ms: float
    queuing_delay_ms: float
    jitter_ms: float
    packet_loss_rate: float = 0.0


@dataclass
class EndToEndTiming:
    """End-to-end timing analysis result"""
    path_name: str
    components: List[ComponentTiming]
    network_hops: List[NetworkTiming]
    
    # Calculated values
    worst_case_latency_ms: float = 0.0
    best_case_latency_ms: float = 0.0
    average_latency_ms: float = 0.0
    jitter_ms: float = 0.0
    
    # Constraint checking
    deadline_met: bool = True
    deadline_violation_probability: float = 0.0
    margin_ms: float = 0.0


class TimingCalculator:
    """
    Calculates and analyzes timing for DDS-based systems
    
    Features:
    - End-to-end latency calculation
    - Deadline analysis
    - Jitter computation
    - Statistical timing analysis
    - Worst-case execution time (WCET) estimation
    """
    
    # Default timing values (in milliseconds)
    DEFAULT_DDS_OVERHEAD_MS = 0.1
    DEFAULT_SERIALIZATION_MS = 0.05
    DEFAULT_DESERIALIZATION_MS = 0.05
    DEFAULT_NETWORK_LATENCY_MS = 0.5
    
    def __init__(self):
        self.constraints: Dict[str, TimingConstraint] = {}
        self.component_timings: Dict[str, ComponentTiming] = {}
        self.network_timings: Dict[str, NetworkTiming] = {}
        
    def add_constraint(self, name: str, constraint: TimingConstraint):
        """Add a timing constraint"""
        self.constraints[name] = constraint
    
    def add_component_timing(self, timing: ComponentTiming):
        """Add component timing characteristics"""
        self.component_timings[timing.name] = timing
    
    def add_network_timing(self, name: str, timing: NetworkTiming):
        """Add network timing characteristics"""
        self.network_timings[name] = timing
    
    def calculate_end_to_end_latency(
        self,
        path_components: List[str],
        network_path: List[str],
        include_overhead: bool = True
    ) -> EndToEndTiming:
        """Calculate end-to-end latency for a path"""
        
        # Get component timings
        components = []
        for comp_name in path_components:
            if comp_name in self.component_timings:
                components.append(self.component_timings[comp_name])
            else:
                # Use default timing
                components.append(ComponentTiming(
                    name=comp_name,
                    worst_case_execution_time_ms=1.0,
                    best_case_execution_time_ms=0.1,
                    average_execution_time_ms=0.5
                ))
        
        # Get network timings
        network_hops = []
        for net_name in network_path:
            if net_name in self.network_timings:
                network_hops.append(self.network_timings[net_name])
            else:
                # Use default timing
                network_hops.append(NetworkTiming(
                    protocol='UDP',
                    bandwidth_mbps=1000.0,
                    propagation_delay_ms=self.DEFAULT_NETWORK_LATENCY_MS / 3,
                    transmission_delay_ms=self.DEFAULT_NETWORK_LATENCY_MS / 3,
                    queuing_delay_ms=self.DEFAULT_NETWORK_LATENCY_MS / 3,
                    jitter_ms=0.1
                ))
        
        # Calculate total latencies
        wcet_sum = sum(c.worst_case_execution_time_ms for c in components)
        bcet_sum = sum(c.best_case_execution_time_ms for c in components)
        avg_sum = sum(c.average_execution_time_ms for c in components)
        
        network_worst = sum(
            n.propagation_delay_ms + n.transmission_delay_ms + n.queuing_delay_ms + n.jitter_ms
            for n in network_hops
        )
        network_best = sum(
            n.propagation_delay_ms + n.transmission_delay_ms * 0.5
            for n in network_hops
        )
        network_avg = sum(
            n.propagation_delay_ms + n.transmission_delay_ms + n.queuing_delay_ms * 0.5
            for n in network_hops
        )
        
        # Add DDS overhead
        overhead = 0.0
        if include_overhead:
            overhead = (len(path_components) - 1) * (
                self.DEFAULT_DDS_OVERHEAD_MS +
                self.DEFAULT_SERIALIZATION_MS +
                self.DEFAULT_DESERIALIZATION_MS
            )
        
        result = EndToEndTiming(
            path_name=' -> '.join(path_components),
            components=components,
            network_hops=network_hops,
            worst_case_latency_ms=wcet_sum + network_worst + overhead,
            best_case_latency_ms=bcet_sum + network_best + overhead,
            average_latency_ms=avg_sum + network_avg + overhead,
            jitter_ms=sum(n.jitter_ms for n in network_hops)
        )
        
        # Check deadline constraints
        deadline_constraint = self.constraints.get('deadline')
        if deadline_constraint:
            result.deadline_met = result.worst_case_latency_ms <= deadline_constraint.value_ms
            result.margin_ms = deadline_constraint.value_ms - result.worst_case_latency_ms
            
            # Calculate violation probability using statistical analysis
            if not result.deadline_met:
                result.deadline_violation_probability = 1.0
            else:
                # Simplified statistical analysis
                std_dev = result.jitter_ms / 2
                if std_dev > 0:
                    z_score = result.margin_ms / std_dev
                    result.deadline_violation_probability = self._normal_cdf(-z_score)
                else:
                    result.deadline_violation_probability = 0.0
        
        return result
    
    def _normal_cdf(self, x: float) -> float:
        """Standard normal cumulative distribution function"""
        return 0.5 * (1 + math.erf(x / math.sqrt(2)))
    
    def calculate_response_time(
        self,
        task_name: str,
        higher_priority_tasks: List[str],
        use_exact_analysis: bool = False
    ) -> Dict[str, float]:
        """
        Calculate worst-case response time using rate monotonic analysis
        
        Args:
            task_name: Name of the task to analyze
            higher_priority_tasks: List of higher priority task names
            use_exact_analysis: Use exact analysis vs approximate
        
        Returns:
            Dictionary with response time calculations
        """
        task = self.component_timings.get(task_name)
        if not task or not task.period_ms:
            return {'error': 'Task timing or period not defined'}
        
        wcet = task.worst_case_execution_time_ms
        period = task.period_ms
        
        # Get higher priority task timings
        hp_tasks = []
        for hp_name in higher_priority_tasks:
            if hp_name in self.component_timings:
                hp_task = self.component_timings[hp_name]
                if hp_task.period_ms:
                    hp_tasks.append(hp_task)
        
        # Sort by period (Rate Monotonic)
        hp_tasks.sort(key=lambda t: t.period_ms)
        
        # Calculate response time
        if use_exact_analysis:
            # Exact response time analysis (iterative)
            response_time = wcet
            prev_response = 0
            
            while response_time != prev_response:
                prev_response = response_time
                interference = sum(
                    math.ceil(response_time / hp.period_ms) * hp.worst_case_execution_time_ms
                    for hp in hp_tasks
                )
                response_time = wcet + interference
                
                if response_time > period:
                    break
        else:
            # Approximate analysis (utilization bound)
            total_utilization = sum(
                hp.worst_case_execution_time_ms / hp.period_ms
                for hp in hp_tasks
            ) + wcet / period
            
            n = len(hp_tasks) + 1
            utilization_bound = n * (2 ** (1/n) - 1)
            
            return {
                'task_utilization': wcet / period,
                'total_utilization': total_utilization,
                'utilization_bound': utilization_bound,
                'schedulable': total_utilization <= utilization_bound,
                'response_time_ms': wcet / (1 - total_utilization + wcet/period) if total_utilization < 1 else float('inf')
            }
        
        return {
            'worst_case_response_ms': response_time,
            'deadline_ms': period,
            'schedulable': response_time <= period,
            'slack_ms': period - response_time
        }
    
    def analyze_qos_deadline(
        self,
        qos_deadline_ms: float,
        component_path: List[str],
        network_path: List[str]
    ) -> Dict[str, Any]:
        """Analyze if QoS deadline can be met"""
        
        e2e = self.calculate_end_to_end_latency(component_path, network_path)
        
        can_meet = e2e.worst_case_latency_ms <= qos_deadline_ms
        margin = qos_deadline_ms - e2e.worst_case_latency_ms
        
        return {
            'qos_deadline_ms': qos_deadline_ms,
            'calculated_wcet_ms': e2e.worst_case_latency_ms,
            'calculated_average_ms': e2e.average_latency_ms,
            'deadline_met': can_meet,
            'margin_ms': margin,
            'margin_percent': (margin / qos_deadline_ms * 100) if qos_deadline_ms > 0 else 0,
            'recommendation': self._get_timing_recommendation(can_meet, margin, qos_deadline_ms)
        }
    
    def _get_timing_recommendation(
        self,
        deadline_met: bool,
        margin_ms: float,
        deadline_ms: float
    ) -> str:
        """Get timing recommendation based on analysis"""
        if not deadline_met:
            return "CRITICAL: Deadline cannot be met. Consider optimizing component execution times or reducing network hops."
        
        margin_percent = abs(margin_ms) / deadline_ms * 100
        
        if margin_percent < 10:
            return "WARNING: Very tight timing margin. System may miss deadlines under heavy load."
        elif margin_percent < 20:
            return "CAUTION: Limited timing margin. Monitor system performance closely."
        else:
            return "OK: Adequate timing margin. System should meet deadlines reliably."
    
    def calculate_buffer_requirements(
        self,
        publisher_rate_hz: float,
        subscriber_rate_hz: float,
        sample_size_bytes: int,
        history_depth: int = 1
    ) -> Dict[str, Any]:
        """Calculate buffer requirements for a topic"""
        
        pub_period_ms = 1000.0 / publisher_rate_hz if publisher_rate_hz > 0 else float('inf')
        sub_period_ms = 1000.0 / subscriber_rate_hz if subscriber_rate_hz > 0 else float('inf')
        
        # Determine buffer strategy
        if subscriber_rate_hz >= publisher_rate_hz:
            # Subscriber reads as fast or faster than publisher
            min_buffer_samples = history_depth
            recommended_buffer_samples = history_depth + 1
        else:
            # Subscriber reads slower than publisher - need buffering
            ratio = publisher_rate_hz / subscriber_rate_hz
            min_buffer_samples = int(math.ceil(ratio)) * history_depth
            recommended_buffer_samples = min_buffer_samples + 2
        
        memory_required_bytes = recommended_buffer_samples * sample_size_bytes
        
        return {
            'publisher_period_ms': pub_period_ms,
            'subscriber_period_ms': sub_period_ms,
            'rate_ratio': publisher_rate_hz / subscriber_rate_hz if subscriber_rate_hz > 0 else float('inf'),
            'history_depth': history_depth,
            'min_buffer_samples': min_buffer_samples,
            'recommended_buffer_samples': recommended_buffer_samples,
            'sample_size_bytes': sample_size_bytes,
            'memory_required_bytes': memory_required_bytes,
            'memory_required_kb': memory_required_bytes / 1024,
            'buffer_strategy': 'QUEUE' if min_buffer_samples > history_depth else 'SIMPLE'
        }
    
    def generate_timing_report(self) -> Dict[str, Any]:
        """Generate comprehensive timing analysis report"""
        
        # Calculate utilization
        total_utilization = sum(
            t.worst_case_execution_time_ms / t.period_ms
            for t in self.component_timings.values()
            if t.period_ms and t.period_ms > 0
        )
        
        # Analyze schedulability
        tasks_with_periods = [
            t for t in self.component_timings.values()
            if t.period_ms and t.period_ms > 0
        ]
        
        n = len(tasks_with_periods)
        utilization_bound = n * (2 ** (1/n) - 1) if n > 0 else 1.0
        
        return {
            'summary': {
                'total_components': len(self.component_timings),
                'total_constraints': len(self.constraints),
                'total_utilization': total_utilization,
                'utilization_bound': utilization_bound,
                'schedulable': total_utilization <= utilization_bound
            },
            'components': [
                {
                    'name': t.name,
                    'wcet_ms': t.worst_case_execution_time_ms,
                    'bcet_ms': t.best_case_execution_time_ms,
                    'period_ms': t.period_ms,
                    'utilization': t.worst_case_execution_time_ms / t.period_ms if t.period_ms else None
                }
                for t in self.component_timings.values()
            ],
            'constraints': [
                {
                    'name': name,
                    'type': c.type.value,
                    'value_ms': c.value_ms,
                    'is_hard': c.is_hard
                }
                for name, c in self.constraints.items()
            ]
        }
    
    def export_to_csv(self, filepath: str):
        """Export timing data to CSV"""
        import csv
        
        with open(filepath, 'w', newline='') as f:
            writer = csv.writer(f)
            writer.writerow([
                'Component', 'WCET (ms)', 'BCET (ms)', 'Average (ms)',
                'Period (ms)', 'Priority', 'Utilization (%)'
            ])
            
            for timing in self.component_timings.values():
                utilization = (timing.worst_case_execution_time_ms / timing.period_ms * 100) \
                    if timing.period_ms else 0
                writer.writerow([
                    timing.name,
                    timing.worst_case_execution_time_ms,
                    timing.best_case_execution_time_ms,
                    timing.average_execution_time_ms,
                    timing.period_ms or 'N/A',
                    timing.priority,
                    f"{utilization:.2f}"
                ])


# Example usage
if __name__ == '__main__':
    calc = TimingCalculator()
    
    # Add component timings
    calc.add_component_timing(ComponentTiming(
        name='SensorPublisher',
        worst_case_execution_time_ms=0.5,
        best_case_execution_time_ms=0.1,
        average_execution_time_ms=0.3,
        period_ms=10.0,
        priority=1
    ))
    
    calc.add_component_timing(ComponentTiming(
        name='Controller',
        worst_case_execution_time_ms=2.0,
        best_case_execution_time_ms=0.5,
        average_execution_time_ms=1.0,
        period_ms=10.0,
        priority=2
    ))
    
    calc.add_component_timing(ComponentTiming(
        name='ActuatorSubscriber',
        worst_case_execution_time_ms=0.5,
        best_case_execution_time_ms=0.1,
        average_execution_time_ms=0.3,
        period_ms=10.0,
        priority=3
    ))
    
    # Add deadline constraint
    calc.add_constraint('deadline', TimingConstraint(
        type=TimingConstraintType.DEADLINE,
        value_ms=5.0,
        is_hard=True
    ))
    
    # Calculate end-to-end latency
    e2e = calc.calculate_end_to_end_latency(
        path_components=['SensorPublisher', 'Controller', 'ActuatorSubscriber'],
        network_path=['eth0', 'eth1']
    )
    
    print("End-to-End Timing Analysis")
    print("=" * 50)
    print(f"Path: {e2e.path_name}")
    print(f"Worst-case latency: {e2e.worst_case_latency_ms:.2f} ms")
    print(f"Best-case latency: {e2e.best_case_latency_ms:.2f} ms")
    print(f"Average latency: {e2e.average_latency_ms:.2f} ms")
    print(f"Jitter: {e2e.jitter_ms:.2f} ms")
    print(f"Deadline met: {e2e.deadline_met}")
    print(f"Margin: {e2e.margin_ms:.2f} ms")