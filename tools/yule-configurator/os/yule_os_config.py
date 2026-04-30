#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
YuleTech OS Configuration Tool Core Engine
基于 AutoSAR OS 标准的配置工具核心引擎

功能:
1. 读取 OS 配置文件 (YAML/JSON 格式)
2. 验证配置合法性 (任务优先级、资源冲突等)
3. 生成 Os_Cfg.h 和 Os_Cfg.c
4. 支持命令行接口
"""

import os
import sys
import json
import yaml
import argparse
import logging
from pathlib import Path
from typing import Dict, List, Any, Optional, Tuple, Set
from dataclasses import dataclass, field, asdict
from enum import Enum, auto
from jinja2 import Environment, FileSystemLoader, Template

# 配置日志
logging.basicConfig(level=logging.INFO, format='%(levelname)s: %(message)s')
logger = logging.getLogger(__name__)


class ScheduleType(Enum):
    """任务调度类型"""
    FULL = "FULL"
    NON = "NON"


class AlarmActionType(Enum):
    """报警动作类型"""
    ACTIVATETASK = "ACTIVATETASK"
    SETEVENT = "SETEVENT"
    ALARMCALLBACK = "ALARMCALLBACK"


@dataclass
class TaskConfig:
    """任务配置"""
    name: str
    priority: int
    activation: int = 1
    autostart: bool = False
    schedule: ScheduleType = ScheduleType.FULL
    events: List[str] = field(default_factory=list)
    resources: List[str] = field(default_factory=list)

    def __post_init__(self):
        if isinstance(self.schedule, str):
            self.schedule = ScheduleType(self.schedule.upper())


@dataclass
class AlarmConfig:
    """报警配置"""
    name: str
    counter: str
    action: AlarmActionType
    task: Optional[str] = None
    event: Optional[str] = None
    callback: Optional[str] = None
    autostart: bool = False
    period: int = 0

    def __post_init__(self):
        if isinstance(self.action, str):
            self.action = AlarmActionType(self.action.upper())


@dataclass
class ResourceConfig:
    """资源配置"""
    name: str
    priority_ceiling: int


@dataclass
class EventConfig:
    """事件配置"""
    name: str
    mask: int


@dataclass
class ExpiryPoint:
    """调度表到期点"""
    offset: int
    tasks: List[str] = field(default_factory=list)
    events: List[Tuple[str, str]] = field(default_factory=list)  # (task, event) pairs


@dataclass
class ScheduleTableConfig:
    """调度表配置"""
    name: str
    periodic: bool = True
    expiry_points: List[ExpiryPoint] = field(default_factory=list)


@dataclass
class OSConfig:
    """OS 完整配置"""
    tasks: List[TaskConfig] = field(default_factory=list)
    alarms: List[AlarmConfig] = field(default_factory=list)
    resources: List[ResourceConfig] = field(default_factory=list)
    events: List[EventConfig] = field(default_factory=list)
    schedule_tables: List[ScheduleTableConfig] = field(default_factory=list)
    os_properties: Dict[str, Any] = field(default_factory=dict)


class ConfigValidator:
    """配置验证器"""

    def __init__(self):
        self.errors: List[str] = []
        self.warnings: List[str] = []

    def validate(self, config: OSConfig) -> Tuple[bool, List[str], List[str]]:
        """验证配置合法性"""
        self.errors = []
        self.warnings = []

        self._validate_tasks(config)
        self._validate_alarms(config)
        self._validate_resources(config)
        self._validate_events(config)
        self._validate_schedule_tables(config)
        self._validate_cross_references(config)

        return len(self.errors) == 0, self.errors, self.warnings

    def _validate_tasks(self, config: OSConfig):
        """验证任务配置"""
        task_names = set()
        priorities = []

        for task in config.tasks:
            # 检查任务名唯一性
            if task.name in task_names:
                self.errors.append(f"Task '{task.name}' 重复定义")
            task_names.add(task.name)

            # 检查优先级范围 (0-255)
            if not 0 <= task.priority <= 255:
                self.errors.append(f"Task '{task.name}' 优先级 {task.priority} 超出范围 (0-255)")
            else:
                priorities.append((task.name, task.priority))

            # 检查激活次数
            if task.activation < 1 or task.activation > 255:
                self.errors.append(f"Task '{task.name}' 激活次数 {task.activation} 超出范围 (1-255)")

        # 检查优先级唯一性 (不同任务应有不同优先级)
        priority_values = [p[1] for p in priorities]
        if len(priority_values) != len(set(priority_values)):
            dupes = [p for p in priorities if priority_values.count(p[1]) > 1]
            self.warnings.append(f"检测到相同优先级的任务: {dupes}")

    def _validate_alarms(self, config: OSConfig):
        """验证报警配置"""
        alarm_names = set()
        counter_names = set()  # 应该由外部定义

        for alarm in config.alarms:
            # 检查报警名唯一性
            if alarm.name in alarm_names:
                self.errors.append(f"Alarm '{alarm.name}' 重复定义")
            alarm_names.add(alarm.name)

            # 检查动作类型与参数匹配
            if alarm.action == AlarmActionType.ACTIVATETASK and not alarm.task:
                self.errors.append(f"Alarm '{alarm.name}' 动作 ACTIVATETASK 需要指定 task")
            if alarm.action == AlarmActionType.SETEVENT:
                if not alarm.task:
                    self.errors.append(f"Alarm '{alarm.name}' 动作 SETEVENT 需要指定 task")
                if not alarm.event:
                    self.errors.append(f"Alarm '{alarm.name}' 动作 SETEVENT 需要指定 event")
            if alarm.action == AlarmActionType.ALARMCALLBACK and not alarm.callback:
                self.errors.append(f"Alarm '{alarm.name}' 动作 ALARMCALLBACK 需要指定 callback")

    def _validate_resources(self, config: OSConfig):
        """验证资源配置"""
        resource_names = set()

        for resource in config.resources:
            # 检查资源名唯一性
            if resource.name in resource_names:
                self.errors.append(f"Resource '{resource.name}' 重复定义")
            resource_names.add(resource.name)

            # 检查优先级天花板
            if resource.priority_ceiling < 0 or resource.priority_ceiling > 255:
                self.errors.append(f"Resource '{resource.name}' 优先级天花板超出范围 (0-255)")

    def _validate_events(self, config: OSConfig):
        """验证事件配置"""
        event_names = set()

        for event in config.events:
            # 检查事件名唯一性
            if event.name in event_names:
                self.errors.append(f"Event '{event.name}' 重复定义")
            event_names.add(event.name)

            # 检查事件掩码
            if event.mask < 0 or event.mask > 0xFFFFFFFF:
                self.errors.append(f"Event '{event.name}' 掩码值超出范围 (0-0xFFFFFFFF)")

    def _validate_schedule_tables(self, config: OSConfig):
        """验证调度表配置"""
        table_names = set()

        for table in config.schedule_tables:
            # 检查调度表名唯一性
            if table.name in table_names:
                self.errors.append(f"ScheduleTable '{table.name}' 重复定义")
            table_names.add(table.name)

            # 检查到期点
            offsets = [ep.offset for ep in table.expiry_points]
            if len(offsets) != len(set(offsets)):
                self.errors.append(f"ScheduleTable '{table.name}' 存在重复的到期点偏移")

    def _validate_cross_references(self, config: OSConfig):
        """验证交叉引用"""
        task_names = {t.name for t in config.tasks}
        event_names = {e.name for e in config.events}
        resource_names = {r.name for r in config.resources}

        # 验证报警引用的任务存在
        for alarm in config.alarms:
            if alarm.task and alarm.task not in task_names:
                self.errors.append(f"Alarm '{alarm.name}' 引用的任务 '{alarm.task}' 不存在")
            if alarm.event and alarm.event not in event_names:
                self.errors.append(f"Alarm '{alarm.name}' 引用的事件 '{alarm.event}' 不存在")

        # 验证任务引用的资源和事件存在
        for task in config.tasks:
            for resource in task.resources:
                if resource not in resource_names:
                    self.errors.append(f"Task '{task.name}' 引用的资源 '{resource}' 不存在")
            for event in task.events:
                if event not in event_names:
                    self.errors.append(f"Task '{task.name}' 引用的事件 '{event}' 不存在")


class ConfigParser:
    """配置文件解析器"""

    @staticmethod
    def load_config(file_path: str) -> OSConfig:
        """加载配置文件"""
        path = Path(file_path)
        
        if not path.exists():
            raise FileNotFoundError(f"配置文件不存在: {file_path}")

        with open(path, 'r', encoding='utf-8') as f:
            if path.suffix in ['.yaml', '.yml']:
                data = yaml.safe_load(f)
            elif path.suffix == '.json':
                data = json.load(f)
            else:
                raise ValueError(f"不支持的文件格式: {path.suffix}")

        return ConfigParser._parse_config(data)

    @staticmethod
    def _parse_config(data: Dict[str, Any]) -> OSConfig:
        """解析配置数据"""
        config = OSConfig()

        # 解析 OS 属性
        config.os_properties = data.get('os', {})

        # 解析任务
        for task_data in data.get('tasks', []):
            config.tasks.append(TaskConfig(**task_data))

        # 解析报警
        for alarm_data in data.get('alarms', []):
            config.alarms.append(AlarmConfig(**alarm_data))

        # 解析资源
        for resource_data in data.get('resources', []):
            config.resources.append(ResourceConfig(**resource_data))

        # 解析事件
        for event_data in data.get('events', []):
            config.events.append(EventConfig(**event_data))

        # 解析调度表
        for table_data in data.get('schedule_tables', []):
            expiry_points = []
            for ep_data in table_data.get('expiry_points', []):
                expiry_points.append(ExpiryPoint(
                    offset=ep_data['offset'],
                    tasks=ep_data.get('tasks', []),
                    events=[tuple(e) for e in ep_data.get('events', [])]
                ))
            config.schedule_tables.append(ScheduleTableConfig(
                name=table_data['name'],
                periodic=table_data.get('periodic', True),
                expiry_points=expiry_points
            ))

        return config


class CodeGenerator:
    """代码生成器"""

    # Os_Cfg.h 模板
    HEADER_TEMPLATE = """/* AUTOGENERATED FILE - DO NOT EDIT */
#ifndef OS_CFG_H
#define OS_CFG_H

#include "Os_Types.h"

/*=============================================================================
 * 任务定义
 *===========================================================================*/
#define OS_TASK_COUNT {{ tasks|length }}
{% for task in tasks %}
#define TASK_{{ task.name|upper }} {{ loop.index0 }}
{% endfor %}
/*=============================================================================
 * 报警定义
 *===========================================================================*/
#define OS_ALARM_COUNT {{ alarms|length }}
{% for alarm in alarms %}
#define ALARM_{{ alarm.name|upper }} {{ loop.index0 }}
{% endfor %}
/*=============================================================================
 * 资源定义
 *===========================================================================*/
#define OS_RESOURCE_COUNT {{ resources|length }}
{% for resource in resources %}
#define RES_{{ resource.name|upper }} {{ loop.index0 }}
#define RES_{{ resource.name|upper }}_PRIORITY {{ resource.priority_ceiling }}
{% endfor %}
/*=============================================================================
 * 事件定义
 *===========================================================================*/
#define OS_EVENT_COUNT {{ events|length }}
{% for event in events %}
#define EVENT_{{ event.name|upper }} ((EventMaskType)0x{{ '%08X'|format(event.mask) }})
{% endfor %}
/*=============================================================================
 * 调度表定义
 *===========================================================================*/
#define OS_SCHEDULETABLE_COUNT {{ schedule_tables|length }}
{% for table in schedule_tables %}
#define SCHEDULETABLE_{{ table.name|upper }} {{ loop.index0 }}
{% endfor %}
/*=============================================================================
 * 任务属性
 *===========================================================================*/
{% for task in tasks %}
#define TASK_{{ task.name|upper }}_PRIORITY {{ task.priority }}
#define TASK_{{ task.name|upper }}_ACTIVATION {{ task.activation }}
#define TASK_{{ task.name|upper }}_AUTOSTART {{ "TRUE" if task.autostart else "FALSE" }}
#define TASK_{{ task.name|upper }}_SCHEDULE {{ task.schedule.value }}
{% endfor %}
/*=============================================================================
 * 报警属性
 *===========================================================================*/
{% for alarm in alarms %}
#define ALARM_{{ alarm.name|upper }}_COUNTER {{ alarm.counter }}
#define ALARM_{{ alarm.name|upper }}_ACTION {{ alarm.action.value }}
{% if alarm.task %}#define ALARM_{{ alarm.name|upper }}_TASK {{ alarm.task }}{% endif %}
{% if alarm.event %}#define ALARM_{{ alarm.name|upper }}_EVENT {{ alarm.event }}{% endif %}
{% endfor %}

#endif /* OS_CFG_H */
"""

    # Os_Cfg.c 模板
    SOURCE_TEMPLATE = """/* AUTOGENERATED FILE - DO NOT EDIT */
#include "Os_Cfg.h"
#include "Os_Internal.h"

/*=============================================================================
 * 任务控制块定义
 *===========================================================================*/
static Os_TaskControlBlockType Os_TaskCB[OS_TASK_COUNT];

const Os_TaskConfigType Os_TaskConfig[OS_TASK_COUNT] = {
{% for task in tasks %}
    {
        .name = "{{ task.name }}",
        .priority = {{ task.priority }},
        .activation = {{ task.activation }},
        .autostart = {{ "TRUE" if task.autostart else "FALSE" }},
        .schedule = {{ task.schedule.value }},
        .entry = Task_{{ task.name }}_Entry,
        .pcb = &Os_TaskCB[{{ loop.index0 }}]
    }{% if not loop.last %},{% endif %}
{% endfor %}
};

/*=============================================================================
 * 报警配置定义
 *===========================================================================*/
static Os_AlarmControlBlockType Os_AlarmCB[OS_ALARM_COUNT];

const Os_AlarmConfigType Os_AlarmConfig[OS_ALARM_COUNT] = {
{% for alarm in alarms %}
    {
        .name = "{{ alarm.name }}",
        .counter = {{ alarm.counter }},
        .action = {{ alarm.action.value }},
        {% if alarm.task %}.task = TASK_{{ alarm.task|upper }},{% endif %}
        {% if alarm.event %}.event = EVENT_{{ alarm.event|upper }},{% endif %}
        {% if alarm.callback %}.callback = {{ alarm.callback }},{% endif %}
        .autostart = {{ "TRUE" if alarm.autostart else "FALSE" }},
        .period = {{ alarm.period }},
        .pcb = &Os_AlarmCB[{{ loop.index0 }}]
    }{% if not loop.last %},{% endif %}
{% endfor %}
};

/*=============================================================================
 * 资源配置定义
 *===========================================================================*/
static Os_ResourceControlBlockType Os_ResourceCB[OS_RESOURCE_COUNT];

const Os_ResourceConfigType Os_ResourceConfig[OS_RESOURCE_COUNT] = {
{% for resource in resources %}
    {
        .name = "{{ resource.name }}",
        .priority_ceiling = {{ resource.priority_ceiling }},
        .pcb = &Os_ResourceCB[{{ loop.index0 }}]
    }{% if not loop.last %},{% endif %}
{% endfor %}
};

/*=============================================================================
 * 调度表配置定义
 *===========================================================================*/
{% for table in schedule_tables %}
static const Os_ScheduleTableExpiryPointType Os_{{ table.name }}_ExpiryPoints[{{ table.expiry_points|length }}] = {
{% for ep in table.expiry_points %}
    {
        .offset = {{ ep.offset }},
        .task_count = {{ ep.tasks|length }},
        .tasks = { {% for task in ep.tasks %}TASK_{{ task|upper }}{% if not loop.last %}, {% endif %}{% endfor %} }
    }{% if not loop.last %},{% endif %}
{% endfor %}
};
{% endfor %}

const Os_ScheduleTableConfigType Os_ScheduleTableConfig[OS_SCHEDULETABLE_COUNT] = {
{% for table in schedule_tables %}
    {
        .name = "{{ table.name }}",
        .periodic = {{ "TRUE" if table.periodic else "FALSE" }},
        .expiry_point_count = {{ table.expiry_points|length }},
        .expiry_points = Os_{{ table.name }}_ExpiryPoints
    }{% if not loop.last %},{% endif %}
{% endfor %}
};

/*=============================================================================
 * 任务入口函数声明 (用户实现)
 *===========================================================================*/
{% for task in tasks %}
extern void Task_{{ task.name }}_Entry(void);
{% endfor %}

/*=============================================================================
 * 报警回调函数声明 (用户实现)
 *===========================================================================*/
{% for alarm in alarms %}
{% if alarm.action.value == "ALARMCALLBACK" %}
extern void {{ alarm.callback }}(void);
{% endif %}
{% endfor %}
"""

    def __init__(self, template_dir: Optional[str] = None):
        self.template_dir = template_dir
        self.env = None
        if template_dir and Path(template_dir).exists():
            self.env = Environment(loader=FileSystemLoader(template_dir))

    def generate_header(self, config: OSConfig, output_path: str):
        """生成 Os_Cfg.h"""
        if self.env:
            template = self.env.get_template('Os_Cfg.h.j2')
        else:
            template = Template(self.HEADER_TEMPLATE)

        content = template.render(
            tasks=config.tasks,
            alarms=config.alarms,
            resources=config.resources,
            events=config.events,
            schedule_tables=config.schedule_tables
        )

        Path(output_path).parent.mkdir(parents=True, exist_ok=True)
        with open(output_path, 'w', encoding='utf-8') as f:
            f.write(content)

        logger.info(f"Generated: {output_path}")

    def generate_source(self, config: OSConfig, output_path: str):
        """生成 Os_Cfg.c"""
        if self.env:
            template = self.env.get_template('Os_Cfg.c.j2')
        else:
            template = Template(self.SOURCE_TEMPLATE)

        content = template.render(
            tasks=config.tasks,
            alarms=config.alarms,
            resources=config.resources,
            events=config.events,
            schedule_tables=config.schedule_tables
        )

        Path(output_path).parent.mkdir(parents=True, exist_ok=True)
        with open(output_path, 'w', encoding='utf-8') as f:
            f.write(content)

        logger.info(f"Generated: {output_path}")


class OSConfigTool:
    """OS 配置工具主类"""

    def __init__(self):
        self.parser = ConfigParser()
        self.validator = ConfigValidator()
        self.generator = CodeGenerator()

    def process(self, config_file: str, output_dir: str) -> bool:
        """处理配置文件并生成代码"""
        try:
            # 1. 加载配置
            logger.info(f"Loading configuration from: {config_file}")
            config = self.parser.load_config(config_file)
            logger.info(f"Loaded {len(config.tasks)} tasks, {len(config.alarms)} alarms, "
                       f"{len(config.resources)} resources, {len(config.events)} events")

            # 2. 验证配置
            logger.info("Validating configuration...")
            is_valid, errors, warnings = self.validator.validate(config)

            for warning in warnings:
                logger.warning(warning)

            if not is_valid:
                logger.error("Configuration validation failed:")
                for error in errors:
                    logger.error(f"  - {error}")
                return False

            logger.info("Configuration validation passed")

            # 3. 生成代码
            logger.info(f"Generating code to: {output_dir}")
            Path(output_dir).mkdir(parents=True, exist_ok=True)

            header_path = Path(output_dir) / "Os_Cfg.h"
            source_path = Path(output_dir) / "Os_Cfg.c"

            self.generator.generate_header(config, str(header_path))
            self.generator.generate_source(config, str(source_path))

            logger.info("Code generation completed successfully")
            return True

        except Exception as e:
            logger.error(f"Processing failed: {e}")
            return False


def main():
    """命令行入口"""
    parser = argparse.ArgumentParser(
        description="YuleTech OS Configuration Tool",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  %(prog)s -c os_config.yaml -o ./generated
  %(prog)s -c os_config.json -o ./output --verbose
        """
    )

    parser.add_argument(
        '-c', '--config',
        required=True,
        help='OS configuration file (YAML or JSON)'
    )

    parser.add_argument(
        '-o', '--output',
        default='./generated',
        help='Output directory for generated files (default: ./generated)'
    )

    parser.add_argument(
        '-t', '--templates',
        help='Custom templates directory (optional)'
    )

    parser.add_argument(
        '-v', '--verbose',
        action='store_true',
        help='Enable verbose output'
    )

    parser.add_argument(
        '--version',
        action='version',
        version='%(prog)s 1.0.0'
    )

    args = parser.parse_args()

    # 设置日志级别
    if args.verbose:
        logging.getLogger().setLevel(logging.DEBUG)

    # 创建工具实例
    tool = OSConfigTool()

    # 设置自定义模板
    if args.templates:
        tool.generator = CodeGenerator(args.templates)

    # 执行处理
    success = tool.process(args.config, args.output)

    sys.exit(0 if success else 1)


if __name__ == '__main__':
    main()
