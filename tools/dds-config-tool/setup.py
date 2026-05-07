from setuptools import setup, find_packages

with open("README.md", "r", encoding="utf-8") as fh:
    long_description = fh.read()

setup(
    name="dds-config-tool",
    version="1.0.0",
    author="YuleTech",
    author_email="support@yuletech.com",
    description="DDS配置工具链 - 生成符合microdds API的C代码",
    long_description=long_description,
    long_description_content_type="text/markdown",
    url="https://github.com/yuletech/micro-dds",
    packages=find_packages(),
    package_data={
        "dds_config_tool": ["templates/*.j2"],
    },
    classifiers=[
        "Development Status :: 4 - Beta",
        "Intended Audience :: Developers",
        "Topic :: Software Development :: Code Generators",
        "License :: OSI Approved :: MIT License",
        "Programming Language :: Python :: 3",
        "Programming Language :: Python :: 3.8",
        "Programming Language :: Python :: 3.9",
        "Programming Language :: Python :: 3.10",
        "Programming Language :: Python :: 3.11",
    ],
    python_requires=">=3.8",
    install_requires=[
        "jinja2>=3.0.0",
    ],
    entry_points={
        "console_scripts": [
            "dds-config-tool=cli:main",
        ],
    },
)
