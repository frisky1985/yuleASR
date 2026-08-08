"""
DDS Config Template Engine
提供Jinja2模板支持，用于生成C代码
"""

from pathlib import Path
from typing import Any, Dict, Optional, Union
from dataclasses import dataclass
from enum import Enum


def _sanitize_c_identifier(name: str) -> str:
    """将名称转换为有效的C标识符"""
    import re
    # 替换非法字符为下划线
    sanitized = re.sub(r'[^a-zA-Z0-9_]', '_', name)
    # 确保不以数字开头
    if sanitized and sanitized[0].isdigit():
        sanitized = '_' + sanitized
    return sanitized


def _to_upper_snake_case(name: str) -> str:
    """转换为全大写下划线命名（用于宏定义）"""
    import re
    # 先插入下划线在大写字母前
    s1 = re.sub('(.)([A-Z][a-z]+)', r'\1_\2', name)
    s2 = re.sub('([a-z0-9])([A-Z])', r'\1_\2', s1)
    # 替换非字母数字为下划线，转大写
    return re.sub(r'[^a-zA-Z0-9_]', '_', s2).upper()


def _to_lower_snake_case(name: str) -> str:
    """转换为全小写下划线命名（用于变量名）"""
    import re
    s1 = re.sub('(.)([A-Z][a-z]+)', r'\1_\2', name)
    s2 = re.sub('([a-z0-9])([A-Z])', r'\1_\2', s1)
    return re.sub(r'[^a-zA-Z0-9_]', '_', s2).lower()


def _format_comment(text: str, width: int = 80) -> str:
    """格式化多行注释，遵守MISRA C规范"""
    if not text:
        return ""
    lines = text.split('\n')
    formatted = []
    for line in lines:
        line = line.strip()
        if len(line) <= width - 4:  # 预留 " * " 的空间
            formatted.append(line)
        else:
            # 简单截断，实际应该按单词分割
            while line:
                chunk = line[:width-4]
                formatted.append(chunk)
                line = line[width-4:]
    return '\n'.join(formatted)


class CodeStyle:
    """代码风格配置"""

    def __init__(self,
                 indent_size: int = 4,
                 use_tabs: bool = False,
                 max_line_length: int = 80,
                 brace_style: str = "allman",  # allman, k&r
                 comment_style: str = "c99"):  # c99, c89
        self.indent_size = indent_size
        self.use_tabs = use_tabs
        self.max_line_length = max_line_length
        self.brace_style = brace_style
        self.comment_style = comment_style

    @property
    def indent(self) -> str:
        return '\t' if self.use_tabs else ' ' * self.indent_size


class TemplateEngine:
    """Jinja2模板引擎包装器，用于DDS C代码生成"""

    def __init__(self,
                 template_dir: Optional[Union[str, Path]] = None,
                 code_style: Optional[CodeStyle] = None):
        """
        初始化模板引擎

        Args:
            template_dir: 模板目录路径，None则使用默认模板
            code_style: 代码风格配置
        """
        self.code_style = code_style or CodeStyle()
        self._env = None
        self._template_dir = None
        self._init_jinja2(template_dir)

    def _init_jinja2(self, template_dir: Optional[Union[str, Path]]) -> None:
        """初始化Jinja2环境"""
        try:
            from jinja2 import Environment, FileSystemLoader, select_autoescape

            if template_dir:
                self._template_dir = Path(template_dir)
            else:
                # 使用默认模板目录（相对于此文件的位置）
                self._template_dir = Path(__file__).parent.parent / "templates"
            
            loader = FileSystemLoader(str(self._template_dir))

            self._env = Environment(
                loader=loader,
                autoescape=select_autoescape(['html', 'xml']),
                trim_blocks=True,
                lstrip_blocks=True,
                keep_trailing_newline=True
            )

            # 注册自定义过滤器
            self._register_filters()
            # 注册全局函数
            self._register_globals()

        except ImportError:
            raise ImportError("Jinja2 is required. Install with: pip install jinja2")

    def _register_filters(self) -> None:
        """注册Jinja2过滤器"""
        self._env.filters['c_identifier'] = _sanitize_c_identifier
        self._env.filters['upper_snake'] = _to_upper_snake_case
        self._env.filters['lower_snake'] = _to_lower_snake_case
        self._env.filters['format_comment'] = _format_comment
        self._env.filters['indent'] = self._indent_filter

    def _register_globals(self) -> None:
        """注册全局函数和变量"""
        self._env.globals['code_style'] = self.code_style
        self._env.globals['now'] = self._get_timestamp
        self._env.globals['misra_compliant'] = True

    def _indent_filter(self, text: str, level: int = 1) -> str:
        """缩进过滤器"""
        indent = self.code_style.indent * level
        lines = text.split('\n')
        return '\n'.join(indent + line if line.strip() else line for line in lines)

    def _get_timestamp(self) -> str:
        """获取当前时间戳"""
        from datetime import datetime
        return datetime.now().strftime('%Y-%m-%d %H:%M:%S')

    def render(self, template_name: str, context: Dict[str, Any]) -> str:
        """
        渲染模板

        Args:
            template_name: 模板文件名
            context: 模板上下文数据

        Returns:
            渲染后的代码字符串
        """
        template = self._env.get_template(template_name)
        return template.render(**context)

    def render_string(self, template_string: str, context: Dict[str, Any]) -> str:
        """
        从字符串渲染模板

        Args:
            template_string: 模板字符串
            context: 模板上下文数据

        Returns:
            渲染后的代码字符串
        """
        from jinja2 import Template
        # 使用当前环境的配置创建模板
        if self._env:
            # 使用现有环境渲染
            template = self._env.from_string(template_string)
            return template.render(**context)
        else:
            # 回退：创建简单模板
            template = Template(template_string)
            return template.render(**context)

    def save_template(self, name: str, content: str) -> Path:
        """
        保存自定义模板

        Args:
            name: 模板名称
            content: 模板内容

        Returns:
            保存路径
        """
        if not self._template_dir:
            raise RuntimeError("Template directory not set")

        template_path = self._template_dir / name
        template_path.write_text(content, encoding='utf-8')
        return template_path

    def list_templates(self) -> list:
        """列出可用模板"""
        return self._env.list_templates()


class TemplateNotFoundError(Exception):
    """模板未找到异常"""
    pass


class TemplateRenderError(Exception):
    """模板渲染异常"""
    pass
