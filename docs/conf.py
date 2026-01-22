# Configuration file for the Sphinx documentation builder.
#
# SAPPOROBDD 2.0 Documentation

import os
import subprocess

# -- Project information -----------------------------------------------------

project = 'SAPPOROBDD 2.0'
copyright = '2024, SAPPOROBDD Team'
author = 'SAPPOROBDD Team'
release = '2.0.0'
version = '2.0'

# -- General configuration ---------------------------------------------------

extensions = [
    'breathe',
    'sphinx.ext.autodoc',
    'sphinx.ext.viewcode',
    'sphinx.ext.todo',
    'sphinx.ext.mathjax',
]

templates_path = ['_templates']
exclude_patterns = ['_build', 'Thumbs.db', '.DS_Store']

# Language settings
language = 'ja'

# -- Options for HTML output -------------------------------------------------

html_theme = 'sphinx_rtd_theme'
html_static_path = ['_static']

html_theme_options = {
    'navigation_depth': 4,
    'collapse_navigation': False,
    'sticky_navigation': True,
}

# -- Breathe configuration ---------------------------------------------------

# Build doxygen XML first
read_the_docs_build = os.environ.get('READTHEDOCS', None) == 'True'

breathe_projects = {
    'SAPPOROBDD2': '_doxygen/xml'
}
breathe_default_project = 'SAPPOROBDD2'
breathe_default_members = ('members', 'undoc-members')

# -- Extension configuration -------------------------------------------------

todo_include_todos = True

# -- Setup for building doxygen on Read the Docs -----------------------------

def run_doxygen(app):
    """Run doxygen to generate XML"""
    docs_dir = os.path.dirname(os.path.abspath(__file__))
    doxygen_dir = os.path.join(docs_dir, '_doxygen')

    # Create output directory
    os.makedirs(doxygen_dir, exist_ok=True)

    # Run doxygen
    try:
        subprocess.run(['doxygen', 'Doxyfile'], cwd=docs_dir, check=True)
        print("Doxygen XML generation completed successfully")
    except subprocess.CalledProcessError as e:
        print(f"Warning: Doxygen failed with error: {e}")
    except FileNotFoundError:
        print("Warning: Doxygen not found, skipping XML generation")

def setup(app):
    app.connect('builder-inited', run_doxygen)
