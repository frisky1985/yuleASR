"""
DDS Config Tool Package

Provides tools for configuring DDS from YAML and generating C code.
"""

from .generator.template_engine import TemplateEngine, CodeStyle
from .generator.code_optimizer import (
    CodeOptimizer,
    NamingConvention,
    CommentGenerator,
    SystemConfig,
    DomainConfig,
    ParticipantConfig,
    TopicConfig,
    QoSProfile
)
from .generator.header_generator import HeaderGenerator, HeaderGenerationOptions
from .generator.source_generator import SourceGenerator, SourceGenerationOptions

__version__ = '1.0.0'

__all__ = [
    # Template Engine
    'TemplateEngine',
    'CodeStyle',
    # Code Optimizer
    'CodeOptimizer',
    'NamingConvention',
    'CommentGenerator',
    # Configuration Models
    'SystemConfig',
    'DomainConfig',
    'ParticipantConfig',
    'TopicConfig',
    'QoSProfile',
    # Generators
    'HeaderGenerator',
    'HeaderGenerationOptions',
    'SourceGenerator',
    'SourceGenerationOptions',
]
