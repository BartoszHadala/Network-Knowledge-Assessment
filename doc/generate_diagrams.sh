#!/bin/bash
# Generate diagrams from PlantUML source

PUML_FILE="diagrams.puml"
OUTPUT_DIR="diagrams_output"

# Check if PlantUML is installed
if ! command -v plantuml &> /dev/null; then
    echo "PlantUML not found. Installing..."
    echo "Please install with: sudo apt install plantuml"
    echo "Or download from: https://plantuml.com/download"
    exit 1
fi

# Create output directory
mkdir -p "$OUTPUT_DIR"

echo "Generating diagrams from $PUML_FILE..."

# Generate PNG images
plantuml -tpng -o "$OUTPUT_DIR" "$PUML_FILE"

# Generate SVG images (scalable)
plantuml -tsvg -o "$OUTPUT_DIR" "$PUML_FILE"

echo "Done! Diagrams saved to $OUTPUT_DIR/"
echo ""
echo "Generated diagrams:"
ls -lh "$OUTPUT_DIR"

# Generate documentation with Pandoc if available
if command -v pandoc &> /dev/null; then
    echo ""
    echo "Generating PDF documentation..."
    pandoc PROJECT_DOCUMENTATION.md -o "$OUTPUT_DIR/documentation.pdf" \
        --pdf-engine=xelatex \
        --toc \
        --toc-depth=3 \
        -V geometry:margin=2cm \
        -V documentclass=report \
        -V colorlinks=true \
        --highlight-style=tango
    
    echo "Generating HTML documentation..."
    pandoc PROJECT_DOCUMENTATION.md -o "$OUTPUT_DIR/documentation.html" \
        --standalone \
        --toc \
        --toc-depth=3 \
        --css=github-markdown.css \
        --highlight-style=tango \
        --metadata title="Network Knowledge Test - Documentation"
    
    echo "Documentation generated in $OUTPUT_DIR/"
else
    echo ""
    echo "Pandoc not found. Skipping PDF/HTML generation."
    echo "Install with: sudo apt install pandoc texlive-xetex"
fi
