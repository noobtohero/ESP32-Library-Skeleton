import os
import gzip
import shutil

import re

def minify_content(content, filename):
    if filename.endswith('.js'):
        # Remove comments
        content = re.sub(r'//.*', '', content)
        content = re.sub(r'/\*.*?\*/', '', content, flags=re.DOTALL)
        # Remove whitespace
        content = re.sub(r'\s+', ' ', content)
        content = re.sub(r'\s*([=+\-*/{}:,;])\s*', r'\1', content)
    elif filename.endswith('.css'):
        # Remove comments
        content = re.sub(r'/\*.*?\*/', '', content, flags=re.DOTALL)
        # Remove whitespace
        content = re.sub(r'\s+', ' ', content)
        content = re.sub(r'\s*([:;{}])\s*', r'\1', content)
    elif filename.endswith('.html'):
        # Remove comments
        content = re.sub(r'<!--.*?-->', '', content, flags=re.DOTALL)
        # Remove whitespace between tags
        content = re.sub(r'>\s+<', '><', content)
        content = re.sub(r'\s+', ' ', content)
    return content.strip()

def compress_file(input_path):
    with open(input_path, 'r', encoding='utf-8') as f_in:
        content = f_in.read()
    
    filename = os.path.basename(input_path)
    minified = minify_content(content, filename)
    print(f"Minified {filename}: {len(content)} -> {len(minified)} bytes")
    
    return gzip.compress(minified.encode('utf-8'))

def bytes_to_c_array(data):
    return ', '.join(f'0x{b:02x}' for b in data)

def generate_header():
    html_path = '../data/index.html'
    css_path = '../data/style.css'
    js_path = '../data/script.js'
    
    if not all(os.path.exists(p) for p in [html_path, css_path, js_path]):
        print("Error: Missing asset files")
        return
        
    with open(html_path, 'r', encoding='utf-8') as f:
        html = f.read()
    with open(css_path, 'r', encoding='utf-8') as f:
        css = f.read()
    with open(js_path, 'r', encoding='utf-8') as f:
        js = f.read()
    
    # Inline CSS
    css_min = minify_content(css, 'style.css')
    html = re.sub(r'<link.*href="style.css".*?>', f'<style>{css_min}</style>', html)
    
    # Inline JS
    js_min = minify_content(js, 'script.js')
    html = re.sub(r'<script.*src="script.js".*?></script>', f'<script>{js_min}</script>', html)
    
    # Minify final HTML
    final_html = minify_content(html, 'index.html')
    compressed_data = gzip.compress(final_html.encode('utf-8'))
    
    header_content = "#ifndef WEBUI_H\n#define WEBUI_H\n\n#include <pgmspace.h>\n\n"
    header_content += f"// Integrated WebUI (HTML + CSS + JS)\n"
    header_content += f"const uint8_t INDEX_HTML_GZ[] PROGMEM = {{\n"
    header_content += bytes_to_c_array(compressed_data)
    header_content += "\n};\n"
    header_content += f"const size_t INDEX_HTML_GZ_LEN = {len(compressed_data)};\n\n"
    header_content += "#endif\n"
    
    output_path = '../src/webui.h'
    with open(output_path, 'w') as f_out:
        f_out.write(header_content)
    
    print("-" * 40)
    print(f"Asset Summary (Integrated):")
    print(f"  Flash Usage: {len(compressed_data)/1024:.2f} KB (GZipped)")
    print("-" * 40)

if __name__ == "__main__":
    generate_header()
