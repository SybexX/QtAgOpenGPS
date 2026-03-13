#!/usr/bin/env python3
"""
SettingsManager Qt6 Code Generator
Generates Qt6 Q_OBJECT_BINDABLE_PROPERTY code from settings_config.txt

Phase 6.0.19 - Complete migration to Qt6 modern patterns
Author: Auto-generated
"""

import os
import sys
import re
from typing import List, Dict, Tuple

def load_properties(config_file: str) -> List[Dict[str, str]]:
    """Load properties from settings_config.txt"""
    properties = []

    if not os.path.exists(config_file):
        print(f"ERROR: Configuration file {config_file} not found")
        return properties

    with open(config_file, 'r', encoding='utf-8') as f:
        for line_num, line in enumerate(f, 1):
            line = line.strip()
            if not line or line.startswith('#'):
                continue

            # Split by | and handle trailing |
            parts = [p.strip() for p in line.split('|') if p.strip()]

            if len(parts) != 4:
                print(f"WARNING: Line {line_num} invalid format (expected 4 fields, got {len(parts)}): {line}")
                continue

            prop = {
                'name': parts[0],
                'iniKey': parts[1],
                'defaultValue': parts[2],
                'type': parts[3]
            }
            properties.append(prop)

    print(f"Loaded {len(properties)} properties from {config_file}")
    return properties

def get_macro_name(prop_type: str) -> str:
    """Get the appropriate macro name for the property type"""
    type_mapping = {
        'QString': 'SETTINGS_PROPERTY_STRING',
        'QColor': 'SETTINGS_PROPERTY_COLOR',
        'QPoint': 'SETTINGS_PROPERTY_POINT',
        'QRect': 'SETTINGS_PROPERTY_RECT',
        'QVector<int>': 'SETTINGS_PROPERTY_VECTOR_INT',
        'METATYPE_QVECTOR_INT': 'SETTINGS_PROPERTY_VECTOR_INT',  # Handle legacy type name
        'QVariantList': 'SETTINGS_PROPERTY_VARIANT_LIST'
    }
    return type_mapping.get(prop_type, 'SETTINGS_PROPERTY')

def capitalize_first_letter(name: str) -> str:
    """Capitalize first letter for Qt setter convention (ab_lineLength -> Ab_lineLength)"""
    if not name:
        return name
    return name[0].upper() + name[1:]

def generate_setter_name(prop_name: str) -> str:
    """Generate Qt-compliant setter name: ab_lineLength -> setAb_lineLength"""
    capitalized = capitalize_first_letter(prop_name)
    return f"set{capitalized}"

def format_default_value(prop_type: str, default_value: str) -> str:
    """Format default value for the specific type"""
    if prop_type == 'QString':
        # Remove existing quotes and QString() wrapper, then add clean quotes
        clean_value = default_value.strip()
        if clean_value.startswith('QString("') and clean_value.endswith('")'):
            clean_value = clean_value[9:-2]  # Remove QString(" and ")
        elif clean_value.startswith('"') and clean_value.endswith('"'):
            clean_value = clean_value[1:-1]  # Remove outer quotes
        return f'"{clean_value}"'
    elif prop_type == 'QColor':
        # Clean QColor wrapper if present
        clean_value = default_value.strip()

        # Remove outer quotes if present
        if clean_value.startswith('"') and clean_value.endswith('"'):
            clean_value = clean_value[1:-1]

        # If it already starts with QColor, return as is
        if clean_value.startswith('QColor(') or clean_value.startswith('QColor::'):
            return clean_value

        # For color names or hex values, add quotes
        if clean_value.startswith('#') or clean_value.isalpha() or ' ' in clean_value:
            return f'QColor("{clean_value}")'

        # For RGB values (e.g., "249,22,22"), create QColor with numbers
        if ',' in clean_value:
            return f'QColor({clean_value})'

        # Default: treat as color name
        return f'QColor("{clean_value}")'
    elif prop_type == 'QPoint':
        clean_value = default_value.strip()
        # If already formatted as QPoint(...), return as is
        if clean_value.startswith('QPoint(') and clean_value.endswith(')'):
            # Extract the values to ensure proper formatting
            inner = clean_value[7:-1].strip()
            if ',' in inner:
                x, y = inner.split(',')
                return f'QPoint({x.strip()}, {y.strip()})'
            return clean_value  # Return as is if no comma found
        # Otherwise, create QPoint from raw values
        if ',' in clean_value:
            x, y = clean_value.split(',')
            return f'QPoint({x.strip()}, {y.strip()})'
        return f'QPoint({clean_value})'
    elif prop_type == 'QRect':
        clean_value = default_value.strip()
        # If already formatted as QRect(...), return as is
        if clean_value.startswith('QRect(') and clean_value.endswith(')'):
            # Extract the values to ensure proper formatting
            inner = clean_value[6:-1].strip()
            if ',' in inner:
                parts = inner.split(',')
                if len(parts) == 4:
                    return f'QRect({parts[0].strip()}, {parts[1].strip()}, {parts[2].strip()}, {parts[3].strip()})'
            return clean_value  # Return as is if format is different
        # Otherwise, create QRect from raw values
        if ',' in clean_value:
            parts = clean_value.split(',')
            if len(parts) == 4:
                return f'QRect({parts[0].strip()}, {parts[1].strip()}, {parts[2].strip()}, {parts[3].strip()})'
        return f'QRect({clean_value})'
    elif prop_type == 'QVector<int>' or prop_type == 'METATYPE_QVECTOR_INT':
        clean_value = default_value.strip()

        # Remove toVariant() wrapper if present
        if clean_value.startswith('toVariant(') and clean_value.endswith(')'):
            clean_value = clean_value[10:-1]  # Remove toVariant( and )

        # Remove QVector<int> wrapper if present
        if clean_value.startswith('QVector<int>{') and clean_value.endswith('}'):
            clean_value = clean_value[13:-1]  # Extract just the values
        elif clean_value.startswith('{') and clean_value.endswith('}'):
            clean_value = clean_value[1:-1]  # Remove braces

        # Return comma-separated values for the variadic macro
        if ',' in clean_value:
            values = [v.strip() for v in clean_value.split(',')]
            return ', '.join(values)
        return clean_value
    elif prop_type == 'QVariantList':
        clean_value = default_value.strip()
        # If empty or just quotes, return empty list
        if not clean_value or clean_value == '""' or clean_value == "''":
            return 'QVariantList()'
        # If it looks like a JSON array, return empty list for now
        # (complex JSON initialization not supported in C++ macros)
        if clean_value.startswith('[') and clean_value.endswith(']'):
            # For complex data, we'll initialize as empty and load from settings
            return 'QVariantList()'
        # Otherwise return empty list
        return 'QVariantList()'
    elif prop_type == 'bool':
        return default_value.lower()
    else:
        return default_value

def generate_properties_declarations_file(properties: List[Dict[str, str]], output_file: str) -> None:
    """Generate settingsmanager_properties.h with DECLARATIONS ONLY (Qt6 Pure)"""
    content = '''// AUTO-GENERATED FILE - DO NOT EDIT
// Generated by generate_settings.py from settings_config.txt
// Phase 6.0.19 - Qt6 Q_OBJECT_BINDABLE_PROPERTY migration
// File 1/3: Q_PROPERTY declarations + public methods

#include "settingsmanager_macros.h"

// Generated Q_PROPERTY declarations with Qt6 bindable support
// Total properties: ''' + str(len(properties)) + '''

'''

    # Group properties by type for better organization
    type_groups = {}
    for prop in properties:
        prop_type = prop['type']
        if prop_type not in type_groups:
            type_groups[prop_type] = []
        type_groups[prop_type].append(prop)

    # Generate DECLARATIONS ONLY grouped by type
    for prop_type, props in type_groups.items():
        content += f'\n// {prop_type} Properties ({len(props)} properties) - DECLARATIONS ONLY\n'

        for prop in props:
            setter_name = generate_setter_name(prop["name"])

            # Use DECLARATIONS macros only
            if prop_type == 'QString':
                content += f'SETTINGS_PROPERTY_STRING_DECLARATIONS({prop["name"]}, {setter_name})\n'
            elif prop_type == 'QColor':
                content += f'SETTINGS_PROPERTY_COLOR_DECLARATIONS({prop["name"]}, {setter_name})\n'
            elif prop_type == 'QPoint':
                content += f'SETTINGS_PROPERTY_POINT_DECLARATIONS({prop["name"]}, {setter_name})\n'
            elif prop_type == 'QRect':
                content += f'SETTINGS_PROPERTY_RECT_DECLARATIONS({prop["name"]}, {setter_name})\n'
            elif prop_type == 'QVector<int>' or prop_type == 'METATYPE_QVECTOR_INT':
                content += f'SETTINGS_PROPERTY_VECTOR_INT_DECLARATIONS({prop["name"]}, {setter_name})\n'
            else:
                # Basic types (double, int, bool)
                content += f'SETTINGS_PROPERTY_DECLARATIONS({prop_type}, {prop["name"]}, {setter_name})\n'

    content += '''
// End of generated property declarations
'''

    with open(output_file, 'w', encoding='utf-8') as f:
        f.write(content)
    print(f"Generated: {output_file}")

def generate_implementations_file(properties: List[Dict[str, str]], output_file: str) -> None:
    """Generate settingsmanager_implementations.cpp with all 377 implementations"""
    content = '''// AUTO-GENERATED FILE - DO NOT EDIT
// Generated by generate_settings.py from settings_config.txt
// Phase 6.0.19 - Qt6 Q_OBJECT_BINDABLE_PROPERTY migration
// IMPLEMENTATIONS for all 377 properties

#include "settingsmanager.h"

// Generated implementations with Qt6 + QSettings persistence
// Total implementations: ''' + str(len(properties)) + '''

'''

    # Group properties by type for better organization
    type_groups = {}
    for prop in properties:
        prop_type = prop['type']
        if prop_type not in type_groups:
            type_groups[prop_type] = []
        type_groups[prop_type].append(prop)

    # Generate IMPLEMENTATIONS grouped by type
    for prop_type, props in type_groups.items():
        content += f'\n// {prop_type} Implementations ({len(props)} properties)\n'

        for prop in props:
            formatted_default = format_default_value(prop_type, prop['defaultValue'])
            setter_name = generate_setter_name(prop["name"])
            iniGroup = prop["iniKey"].split('/')[0]

            # Use IMPLEMENTATION macros
            if prop_type == 'QString':
                content += f'SETTINGS_PROPERTY_STRING_IMPL({prop["name"]}, {iniGroup}, "{prop["iniKey"]}", {formatted_default}, {setter_name})\n'
            elif prop_type == 'QColor':
                content += f'SETTINGS_PROPERTY_COLOR_IMPL({prop["name"]}, {iniGroup}, "{prop["iniKey"]}", {formatted_default}, {setter_name})\n'
            elif prop_type == 'QPoint':
                content += f'SETTINGS_PROPERTY_POINT_IMPL({prop["name"]}, {iniGroup}, "{prop["iniKey"]}", {formatted_default}, {setter_name})\n'
            elif prop_type == 'QRect':
                content += f'SETTINGS_PROPERTY_RECT_IMPL({prop["name"]}, {iniGroup}, "{prop["iniKey"]}", {formatted_default}, {setter_name})\n'
            elif prop_type == 'QVector<int>' or prop_type == 'METATYPE_QVECTOR_INT':
                # Generate manual implementation for QVector<int> to avoid macro issues
                iniGroup = prop["iniKey"].split('/')[0]
                content += f'''QVector<int> SettingsManager::{prop["name"]}() const {{
    return m_{prop["name"]}.value();
}}
void SettingsManager::{setter_name}(const QVector<int>& value) {{
    QStringList strList;
    for (int i : value) strList << QString::number(i);
    m_{prop["name"]}.setValue(value);
    m_qsettings->setValue("{prop["iniKey"]}", strList);
    m_qsettings->sync();
    emit {iniGroup}GroupChanged();
}}
QBindable<QVector<int>> SettingsManager::bindable{prop["name"]}() {{
    return &m_{prop["name"]};
}}
'''
            else:
                # Basic types (double, int, bool)
                content += f'SETTINGS_PROPERTY_IMPL({prop_type}, {prop["name"]}, {iniGroup}, "{prop["iniKey"]}", {formatted_default}, {setter_name})\n'

        content += '\n'

    content += '''
// End of generated implementations
'''

    with open(output_file, 'w', encoding='utf-8') as f:
        f.write(content)
    print(f"Generated: {output_file}")

def generate_initialization_file(properties: List[Dict[str, str]], output_file: str) -> None:
    """Generate initializeFromSettings() method with correct default values"""
    content = '''// AUTO-GENERATED FILE - DO NOT EDIT
// Generated by generate_settings.py from settings_config.txt
// Phase 6.0.19 - Qt6 Q_OBJECT_BINDABLE_PROPERTY migration
// CRITICAL: QSettings loading with proper default values

#include "settingsmanager.h"

void SettingsManager::initializeFromSettings()
{
    // Load all ''' + str(len(properties)) + ''' properties from QSettings with default fallback
    // IMPORTANT: Uses QSettings second parameter for defaults (not hardcoded 0/false)

'''

    for prop in properties:
        formatted_default = format_default_value(prop['type'], prop['defaultValue'])

        if prop['type'] == 'QVector<int>':
            # Convert QStringList from INI to QVector<int> with proper default
            values = [v.strip() for v in prop['defaultValue'].split(',') if v.strip()]
            default_list = ', '.join([f'"{v}"' for v in values])
            content += f'''    {{
        QStringList defaultList({{{default_list}}});
        QStringList list = m_qsettings->value("{prop["iniKey"]}", defaultList).toStringList();
        QVector<int> vector;
        for (const QString& str : std::as_const(list)) {{
            bool ok;
            int val = str.toInt(&ok);
            if (ok) vector.append(val);
        }}
        m_{prop["name"]}.setValue(vector);
    }}
'''
        elif prop['type'] == 'QString':
            content += f'    m_{prop["name"]}.setValue(m_qsettings->value("{prop["iniKey"]}", {formatted_default}).toString());\n'
        elif prop['type'] == 'QVariantList':
            content += f'    m_{prop["name"]}.setValue(m_qsettings->value("{prop["iniKey"]}", {formatted_default}).toList());\n'
        elif prop['type'] == 'bool':
            content += f'    m_{prop["name"]}.setValue(m_qsettings->value("{prop["iniKey"]}", {formatted_default}).toBool());\n'
        elif prop['type'] == 'int':
            content += f'    m_{prop["name"]}.setValue(m_qsettings->value("{prop["iniKey"]}", {formatted_default}).toInt());\n'
        elif prop['type'] == 'double':
            content += f'    m_{prop["name"]}.setValue(m_qsettings->value("{prop["iniKey"]}", {formatted_default}).toDouble());\n'
        elif prop['type'] == 'float':
            content += f'    m_{prop["name"]}.setValue(static_cast<float>(m_qsettings->value("{prop["iniKey"]}", {formatted_default}).toDouble()));\n'
        elif prop['type'] == 'QColor':
            content += f'    m_{prop["name"]}.setValue(m_qsettings->value("{prop["iniKey"]}", QVariant::fromValue({formatted_default})).value<QColor>());\n'
        elif prop['type'] == 'QPoint':
            content += f'    m_{prop["name"]}.setValue(m_qsettings->value("{prop["iniKey"]}", {formatted_default}).toPoint());\n'
        elif prop['type'] == 'QRect':
            content += f'    m_{prop["name"]}.setValue(m_qsettings->value("{prop["iniKey"]}", {formatted_default}).toRect());\n'
        else:
            content += f'    m_{prop["name"]}.setValue(m_qsettings->value("{prop["iniKey"]}", {formatted_default}).value<{prop["type"]}>());\n'

    content += '''
    // All properties loaded with proper defaults
    qDebug() << "SettingsManager: initialized" << ''' + str(len(properties)) + ''' << "properties from" << m_qsettings->fileName();
}
'''

    with open(output_file, 'w', encoding='utf-8') as f:
        f.write(content)
    print(f"Generated: {output_file}")

def generate_members_file(properties: List[Dict[str, str]], output_file: str) -> None:
    """Generate settingsmanager_members.h - Q_OBJECT_BINDABLE_PROPERTY members"""
    content = '''// AUTO-GENERATED FILE - DO NOT EDIT
// Generated by generate_settings.py from settings_config.txt
// Phase 6.0.19 - Qt6 Q_OBJECT_BINDABLE_PROPERTY migration
// File 3/3: Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS members for private: section

// Generated property members with Qt6 bindable support
// Total members: ''' + str(len(properties)) + '''

'''

    for prop in properties:
        formatted_default = format_default_value(prop['type'], prop['defaultValue'])

        if prop['type'] == 'QVector<int>' or prop['type'] == 'METATYPE_QVECTOR_INT':
            # Special handling for QVector<int> - use formatted_default to remove toVariant()
            clean_value = formatted_default
            # Extract values from formatted_default which already cleaned toVariant()
            values = [v.strip() for v in clean_value.split(',') if v.strip()]
            vector_init = ', '.join(values) if values else ''
            content += f'Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(SettingsManager, QVector<int>, m_{prop["name"]}, (QVector<int>{{{vector_init}}}), &SettingsManager::{prop["name"]}Changed)\n'
        else:
            content += f'Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(SettingsManager, {prop["type"]}, m_{prop["name"]}, {formatted_default}, &SettingsManager::{prop["name"]}Changed)\n'

    content += '''
// End of generated property members
'''

    with open(output_file, 'w', encoding='utf-8') as f:
        f.write(content)
    print(f"Generated: {output_file}")

def update_signals_in_header_file(properties: List[Dict[str, str]]) -> None:
    """Update signals section directly in settingsmanager.h"""
    header_file = "classes/settingsmanager.h"

    try:
        # Read the current file
        with open(header_file, 'r', encoding='utf-8') as f:
            content = f.read()

        # Generate the new signals content
        signals_content = f"signals:\n"
        signals_content += f"    // ===== GENERATED NOTIFY SIGNALS ({len(properties)} signals) =====\n"
        signals_content += f"    // Required for Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS signal references\n"

        # Get groups
        groups = []
        for prop in properties:
            key = prop["iniKey"].split('/')[0]
            if key in groups: continue
            groups.append(key);


        # Add all signals
        for prop in properties:
            signals_content += f'    void {prop["name"]}Changed();\n'

        for group in groups:
            signals_content += f'    void {group}GroupChanged();\n'

        # Replace the existing signals section using regex
        pattern = r'signals:\s*\n.*?// Required for Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS signal references\n.*?(?=\n\nprivate:|\nprivate:)'
        new_content = re.sub(pattern, signals_content, content, flags=re.DOTALL)

        # Write the updated content
        with open(header_file, 'w', encoding='utf-8') as f:
            f.write(new_content)

        print(f"Updated: {header_file} with {len(properties)} signals")

    except Exception as e:
        print(f"ERROR updating signals in {header_file}: {e}")
        raise

def main():
    """Main generation function"""
    print("SettingsManager Qt6 Code Generator - Phase 6.0.19")
    print("=" * 50)

    config_file = "settings_config.txt"
    if len(sys.argv) > 1:
        config_file = sys.argv[1]

    # Load properties
    properties = load_properties(config_file)
    if not properties:
        print("ERROR: No properties loaded, exiting")
        return 1

    # Generate 4 files + update macro in settingsmanager_macros.h
    output_files = [
        ("classes/settingsmanager_properties.h", generate_properties_declarations_file),
        ("classes/settingsmanager_members.h", generate_members_file),
        ("classes/settingsmanager_implementations.cpp", generate_implementations_file),
        ("classes/settingsmanager_initialize.cpp", generate_initialization_file)
    ]

    print(f"\nGenerating {len(output_files)} files...")
    for filename, generator_func in output_files:
        try:
            generator_func(properties, filename)
        except Exception as e:
            print(f"ERROR generating {filename}: {e}")
            raise e

    # Update signals directly in settingsmanager.h
    try:
        update_signals_in_header_file(properties)
    except Exception as e:
        print(f"ERROR updating signals in header: {e}")
        return 1

    print(f"\n[SUCCESS] Generated all {len(output_files)} files")
    print(f"[INFO] Total properties processed: {len(properties)}")

    # Type summary
    type_counts = {}
    for prop in properties:
        prop_type = prop['type']
        type_counts[prop_type] = type_counts.get(prop_type, 0) + 1

    print("\n[SUMMARY] Property types:")
    for prop_type, count in sorted(type_counts.items()):
        print(f"   {prop_type}: {count}")

    return 0

if __name__ == "__main__":
    sys.exit(main())
