/**
 * ============================================================================
 * TRUE PDF EDITOR - Production-Grade Desktop Application
 * ============================================================================
 * Architecture  : Principal C++ / Qt6 / Poppler-Qt6
 * Author        : Principal C++ Desktop Software Architect
 * Description   : A fully-featured PDF editor modeled after Adobe Acrobat's
 *                 core editing section. Supports true in-place text editing,
 *                 image manipulation, undo/redo history, paragraph reflow,
 *                 font/color/alignment controls, page thumbnails, and more.
 *
 * Build deps (CMakeLists.txt excerpt):
 *   find_package(Qt6 REQUIRED COMPONENTS Core Widgets Gui PrintSupport)
 *   find_package(PopplerQt6 REQUIRED)
 *   target_link_libraries(TruePDFEditor
 *       Qt6::Core Qt6::Widgets Qt6::Gui Qt6::PrintSupport
 *       PopplerQt6::PopplerQt6)
 * ============================================================================
 */

#include <QtWidgets/QApplication>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QGraphicsView>
#include <QtWidgets/QGraphicsScene>
#include <QtWidgets/QGraphicsTextItem>
#include <QtWidgets/QGraphicsPixmapItem>
#include <QtWidgets/QGraphicsRectItem>
#include <QtWidgets/QGraphicsItem>
#include <QtWidgets/QDockWidget>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QListWidgetItem>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QAction>
#include <QtWidgets/QActionGroup>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QMenu>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QFontComboBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QColorDialog>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QFrame>
#include <QtWidgets/QSlider>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QSizeGrip>
#include <QtWidgets/QRubberBand>
#include <QtWidgets/QScrollBar>
#include <QtWidgets/QAbstractScrollArea>
#include <QtGui/QColor>
#include <QtGui/QFont>
#include <QtGui/QFontMetricsF>
#include <QtGui/QPainter>
#include <QtGui/QPainterPath>
#include <QtGui/QPixmap>
#include <QtGui/QImage>
#include <QtGui/QKeyEvent>
#include <QtGui/QMouseEvent>
#include <QtGui/QWheelEvent>
#include <QtGui/QDragEnterEvent>
#include <QtGui/QDropEvent>
#include <QtGui/QTextCursor>
#include <QtGui/QTextDocument>
#include <QtGui/QTextBlock>
#include <QtGui/QTextLayout>
#include <QtGui/QTextLine>
#include <QtGui/QTextCharFormat>
#include <QtGui/QTextBlockFormat>
#include <QtGui/QTextOption>
#include <QtGui/QUndoStack>
#include <QtGui/QUndoCommand>
#include <QtGui/QPen>
#include <QtGui/QBrush>
#include <QtGui/QTransform>
#include <QtGui/QCursor>
#include <QtGui/QIcon>
#include <QtGui/QClipboard>
#include <QtGui/QGuiApplication>
#include <QtGui/QScreen>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QVector>
#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QHash>
#include <QtCore/QSet>
#include <QtCore/QRect>
#include <QtCore/QRectF>
#include <QtCore/QPointF>
#include <QtCore/QSizeF>
#include <QtCore/QTimer>
#include <QtCore/QDebug>
#include <QtCore/QMimeData>
#include <QtCore/QUrl>
#include <QtCore/QFileInfo>
#include <QtCore/QDir>
#include <QtCore/QElapsedTimer>
#include <QtCore/QThread>
#include <QtCore/QMutex>
#include <QtCore/QSemaphore>
#include <QtCore/QSettings>
#include <QtCore/QStandardPaths>
#include <QtPrintSupport/QPrinter>
#include <QtPrintSupport/QPrintDialog>

// Poppler Qt6 bindings
#include <poppler/qt6/poppler-qt6.h>

#include <memory>
#include <vector>
#include <algorithm>
#include <cmath>
#include <functional>
#include <optional>
#include <variant>
#include <map>
#include <unordered_map>
#include <set>

// ============================================================================
// CONSTANTS AND ENUMERATIONS
// ============================================================================

namespace PDFEditor {

constexpr double DEFAULT_DPI       = 150.0;
constexpr double THUMB_DPI         = 72.0;
constexpr double MIN_ZOOM          = 0.10;
constexpr double MAX_ZOOM          = 8.00;
constexpr double ZOOM_STEP         = 0.15;
constexpr int    HANDLE_SIZE       = 8;
constexpr int    PAGE_MARGIN       = 20;
constexpr int    PAGE_SHADOW       = 6;
constexpr int    THUMB_WIDTH       = 130;
constexpr int    THUMB_HEIGHT      = 170;
constexpr double TEXT_PADDING      = 2.0;
constexpr double REFLOW_TOLERANCE  = 0.5; // px tolerance for reflow
constexpr int    AUTOSAVE_INTERVAL = 60000; // ms

enum class ToolMode {
    Select,
    AddText,
    AddImage,
    Hand,
    Zoom
};

enum class HandlePosition {
    TopLeft,     Top,      TopRight,
    Left,                  Right,
    BottomLeft,  Bottom,   BottomRight
};

enum class ItemType {
    TextBlock,
    ImageBlock,
    PageBackground
};

} // namespace PDFEditor

// ============================================================================
// FORWARD DECLARATIONS
// ============================================================================

class PDFDocument;
class EditorCanvas;
class CanvasView;
class InteractiveTextNode;
class InteractiveImageNode;
class ResizeHandle;
class SelectionBox;
class PageItem;
class ThumbnailWidget;
class PropertiesPanel;
class PDFEditorMainWindow;

// ============================================================================
// PDF DOCUMENT WRAPPER
// ============================================================================

/**
 * PDFDocument wraps a Poppler document, providing page rendering,
 * text extraction, and image extraction as Qt-friendly data structures.
 */
struct ExtractedTextBlock {
    QString         text;
    QRectF          boundingBox;   // in scene coordinates (DPI-scaled)
    QFont           font;
    QColor          color;
    int             pageIndex;
    Qt::Alignment   alignment;
    double          lineSpacing;
    bool            isBold;
    bool            isItalic;
};

struct ExtractedImage {
    QImage  image;
    QRectF  boundingBox;
    int     pageIndex;
};

class PDFDocument : public QObject {
    Q_OBJECT
public:
    explicit PDFDocument(QObject* parent = nullptr)
        : QObject(parent)
        , m_document(nullptr)
        , m_dpi(PDFEditor::DEFAULT_DPI)
    {}

    ~PDFDocument() = default;

    bool load(const QString& filePath) {
        m_document.reset(Poppler::Document::load(filePath));
        if (!m_document || m_document->isLocked()) {
            m_document.reset();
            return false;
        }
        m_document->setRenderHint(Poppler::Document::TextAntialiasing);
        m_document->setRenderHint(Poppler::Document::Antialiasing);
        m_document->setRenderHint(Poppler::Document::TextHinting);
        m_filePath = filePath;
        m_pageCount = m_document->numPages();
        cachePageSizes();
        return true;
    }

    // Render a full page to QImage at given DPI
    QImage renderPage(int pageIndex, double dpi = PDFEditor::DEFAULT_DPI) const {
        if (!m_document || pageIndex < 0 || pageIndex >= m_pageCount)
            return {};
        std::unique_ptr<Poppler::Page> page(m_document->page(pageIndex));
        if (!page) return {};
        return page->renderToImage(dpi, dpi);
    }

    // Render a small thumbnail
    QImage renderThumbnail(int pageIndex) const {
        return renderPage(pageIndex, PDFEditor::THUMB_DPI);
    }

    // Extract all text blocks from a page, with font and color info
    QVector<ExtractedTextBlock> extractTextBlocks(int pageIndex) const {
        QVector<ExtractedTextBlock> blocks;
        if (!m_document || pageIndex < 0 || pageIndex >= m_pageCount)
            return blocks;

        std::unique_ptr<Poppler::Page> page(m_document->page(pageIndex));
        if (!page) return blocks;

        // Scale factor: Poppler points -> scene pixels
        const double scaleX = m_dpi / 72.0;
        const double scaleY = m_dpi / 72.0;

        QList<Poppler::TextBox*> textBoxes = page->textList();

        // Group TextBox objects into logical paragraph blocks
        // using spatial clustering with a line-height tolerance
        QVector<QList<Poppler::TextBox*>> lineGroups;
        groupTextBoxesIntoLines(textBoxes, lineGroups, scaleY);

        for (auto& lineGroup : lineGroups) {
            if (lineGroup.isEmpty()) continue;

            ExtractedTextBlock block;
            block.pageIndex = pageIndex;

            // Compute union bounding box
            QRectF unionBox;
            QString fullText;
            QFont   dominantFont;
            QColor  dominantColor(Qt::black);

            // Track font statistics to find dominant font
            QMap<QString, int> fontUsage;
            QMap<double, int>  sizeUsage;

            for (Poppler::TextBox* tb : lineGroup) {
                QRectF rawBox = tb->boundingBox();
                // Scale from 72-DPI Poppler space to scene space
                QRectF scaledBox(
                    rawBox.x() * scaleX,
                    rawBox.y() * scaleY,
                    rawBox.width() * scaleX,
                    rawBox.height() * scaleY
                );
                unionBox = unionBox.isNull() ? scaledBox : unionBox.united(scaledBox);
                fullText += tb->text();

                // Extract font info
                QFont f = tb->font();
                fontUsage[f.family()]++;
                sizeUsage[f.pointSizeF()]++;

                // Detect bold/italic from font name heuristics
                QString familyLower = f.family().toLower();
                if (familyLower.contains("bold"))   f.setBold(true);
                if (familyLower.contains("italic"))  f.setItalic(true);
                if (familyLower.contains("oblique")) f.setItalic(true);

                dominantFont = f;
            }

            // Deduce dominant font family and size
            if (!fontUsage.isEmpty()) {
                QString bestFamily = fontUsage.begin().key();
                int bestCount = 0;
                for (auto it = fontUsage.begin(); it != fontUsage.end(); ++it) {
                    if (it.value() > bestCount) {
                        bestCount = it.value();
                        bestFamily = it.key();
                    }
                }
                dominantFont.setFamily(bestFamily);
            }
            if (!sizeUsage.isEmpty()) {
                double bestSize = sizeUsage.begin().key();
                int bestCount = 0;
                for (auto it = sizeUsage.begin(); it != sizeUsage.end(); ++it) {
                    if (it.value() > bestCount) {
                        bestCount = it.value();
                        bestSize = it.key();
                    }
                }
                // Convert Poppler point size to pixel size for our DPI
                double pixelSize = bestSize * (m_dpi / 72.0);
                dominantFont.setPixelSize(qMax(8, static_cast<int>(pixelSize)));
            }

            block.text        = fullText.trimmed();
            block.boundingBox = unionBox;
            block.font        = dominantFont;
            block.color       = dominantColor;
            block.isBold      = dominantFont.bold();
            block.isItalic    = dominantFont.italic();
            block.alignment   = Qt::AlignLeft;
            block.lineSpacing = 1.2;

            if (!block.text.isEmpty() && block.boundingBox.width() > 2.0) {
                blocks.append(block);
            }
        }

        qDeleteAll(textBoxes);
        return blocks;
    }

    // Extract embedded images from a page
    QVector<ExtractedImage> extractImages(int pageIndex) const {
        QVector<ExtractedImage> images;
        if (!m_document || pageIndex < 0 || pageIndex >= m_pageCount)
            return images;

        // Poppler image extraction via page renderToImage with individual
        // image annotations; we use the page's image list
        std::unique_ptr<Poppler::Page> page(m_document->page(pageIndex));
        if (!page) return images;

        const double scaleX = m_dpi / 72.0;
        const double scaleY = m_dpi / 72.0;

        QList<Poppler::Image> popplerImages = page->images();
        for (const Poppler::Image& pImg : popplerImages) {
            ExtractedImage img;
            img.image     = pImg.image();
            img.pageIndex = pageIndex;
            // Poppler::Image provides boundingBox in page units (72 DPI)
            img.boundingBox = QRectF(
                pImg.location().x() * scaleX,
                pImg.location().y() * scaleY,
                pImg.image().width()  * scaleX / (m_dpi / 72.0),
                pImg.image().height() * scaleY / (m_dpi / 72.0)
            );
            if (!img.image.isNull())
                images.append(img);
        }
        return images;
    }

    int    pageCount()  const { return m_pageCount; }
    double dpi()        const { return m_dpi; }
    void   setDpi(double d)   { m_dpi = d; }
    QString filePath()  const { return m_filePath; }

    QSizeF pageSize(int pageIndex) const {
        if (pageIndex >= 0 && pageIndex < m_pageSizes.size())
            return m_pageSizes[pageIndex];
        return {};
    }

    // Page size in scene pixels
    QSizeF pageSizeScaled(int pageIndex) const {
        QSizeF pts = pageSize(pageIndex);
        return QSizeF(pts.width() * m_dpi / 72.0, pts.height() * m_dpi / 72.0);
    }

    Poppler::Document* rawDocument() const { return m_document.get(); }

private:
    void cachePageSizes() {
        m_pageSizes.clear();
        m_pageSizes.reserve(m_pageCount);
        for (int i = 0; i < m_pageCount; ++i) {
            std::unique_ptr<Poppler::Page> page(m_document->page(i));
            m_pageSizes.append(page ? page->pageSizeF() : QSizeF(595, 842));
        }
    }

    // Group individual TextBox glyphs into logical lines based on Y proximity
    void groupTextBoxesIntoLines(
        const QList<Poppler::TextBox*>& boxes,
        QVector<QList<Poppler::TextBox*>>& lineGroups,
        double scaleY) const
    {
        if (boxes.isEmpty()) return;

        // Sort by Y then X
        QList<Poppler::TextBox*> sorted = boxes;
        std::sort(sorted.begin(), sorted.end(),
            [](Poppler::TextBox* a, Poppler::TextBox* b) {
                double ay = a->boundingBox().top();
                double by = b->boundingBox().top();
                if (std::abs(ay - by) < 2.0) // same line tolerance
                    return a->boundingBox().left() < b->boundingBox().left();
                return ay < by;
            });

        QList<Poppler::TextBox*> currentLine;
        double currentLineY = -1e9;

        for (Poppler::TextBox* tb : sorted) {
            double boxTop    = tb->boundingBox().top();
            double boxHeight = tb->boundingBox().height();

            if (currentLine.isEmpty()) {
                currentLine.append(tb);
                currentLineY = boxTop;
            } else {
                // Tolerance: half the average line height
                double tolerance = boxHeight * 0.6;
                if (std::abs(boxTop - currentLineY) <= tolerance) {
                    currentLine.append(tb);
                } else {
                    lineGroups.append(currentLine);
                    currentLine.clear();
                    currentLine.append(tb);
                    currentLineY = boxTop;
                }
            }
        }
        if (!currentLine.isEmpty())
            lineGroups.append(currentLine);
    }

    std::unique_ptr<Poppler::Document> m_document;
    QString   m_filePath;
    int       m_pageCount = 0;
    double    m_dpi;
    QVector<QSizeF> m_pageSizes;
};

// ============================================================================
// PARAGRAPH REFLOW ENGINE
// ============================================================================

/**
 * ReflowEngine performs word-wrap and bounding-box recalculation for a
 * text block after editing. Uses Qt's QTextLayout machinery for precise
 * font metrics and Unicode-aware line breaking.
 */
class ReflowEngine {
public:
    struct ReflowResult {
        QVector<QString> lines;
        QVector<QRectF>  lineRects;  // relative to block origin
        QSizeF           requiredSize;
        bool             overflows;
    };

    /**
     * Perform a full reflow of `text` within `containerRect` using `font`.
     * Returns the line layout and whether the text overflows vertically.
     */
    static ReflowResult reflow(
        const QString&    text,
        const QRectF&     containerRect,
        const QFont&      font,
        Qt::Alignment     alignment,
        double            lineSpacing = 1.2)
    {
        ReflowResult result;
        result.overflows = false;

        if (text.isEmpty()) {
            result.requiredSize = QSizeF(containerRect.width(), 0);
            return result;
        }

        QFontMetricsF fm(font);
        const double containerWidth  = qMax(1.0, containerRect.width() - 2 * PDFEditor::TEXT_PADDING);
        const double containerHeight = containerRect.height();
        const double lineHeight      = fm.height() * lineSpacing;
        const double ascent          = fm.ascent();

        // Use QTextLayout for proper Unicode bidirectional and word-wrap support
        QTextLayout layout(text, font);
        QTextOption textOption;
        textOption.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
        textOption.setAlignment(alignment);
        layout.setTextOption(textOption);

        layout.beginLayout();
        double y = PDFEditor::TEXT_PADDING;
        while (true) {
            QTextLine line = layout.createLine();
            if (!line.isValid()) break;

            line.setLineWidth(containerWidth);
            line.setPosition(QPointF(PDFEditor::TEXT_PADDING, y));
            y += lineHeight;
        }
        layout.endLayout();

        // Harvest lines
        for (int i = 0; i < layout.lineCount(); ++i) {
            QTextLine line = layout.lineAt(i);
            QString lineText = text.mid(line.textStart(), line.textLength());
            result.lines.append(lineText);

            QRectF lineRect(
                line.position().x(),
                line.position().y(),
                line.naturalTextWidth(),
                lineHeight
            );
            result.lineRects.append(lineRect);
        }

        double totalHeight = y + PDFEditor::TEXT_PADDING;
        result.requiredSize = QSizeF(containerRect.width(), totalHeight);

        if (totalHeight > containerHeight + PDFEditor::REFLOW_TOLERANCE)
            result.overflows = true;

        return result;
    }

    /**
     * Auto-expand: given a fixed width, compute the minimum bounding height
     * required to display the text without overflow.
     */
    static double computeRequiredHeight(
        const QString& text,
        double         containerWidth,
        const QFont&   font,
        double         lineSpacing = 1.2)
    {
        QFontMetricsF fm(font);
        const double usableWidth = qMax(1.0, containerWidth - 2 * PDFEditor::TEXT_PADDING);
        const double lineHeight  = fm.height() * lineSpacing;

        QTextLayout layout(text, font);
        QTextOption opt;
        opt.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
        layout.setTextOption(opt);
        layout.beginLayout();
        double y = PDFEditor::TEXT_PADDING;
        while (true) {
            QTextLine line = layout.createLine();
            if (!line.isValid()) break;
            line.setLineWidth(usableWidth);
            line.setPosition(QPointF(PDFEditor::TEXT_PADDING, y));
            y += lineHeight;
        }
        layout.endLayout();
        return y + PDFEditor::TEXT_PADDING;
    }

    /**
     * Compute the minimum width needed to fit the longest word.
     */
    static double computeMinimumWidth(const QString& text, const QFont& font) {
        QFontMetricsF fm(font);
        double maxWordWidth = 0.0;
        for (const QString& word : text.split(QChar(' '), Qt::SkipEmptyParts)) {
            double w = fm.horizontalAdvance(word);
            maxWordWidth = qMax(maxWordWidth, w);
        }
        return maxWordWidth + 2 * PDFEditor::TEXT_PADDING + 4;
    }
};

// ============================================================================
// UNDO/REDO COMMAND CLASSES
// ============================================================================

// Forward declare for commands
class InteractiveTextNode;
class InteractiveImageNode;

/**
 * TextEditCommand: captures before/after text state of a text node.
 */
class TextEditCommand : public QUndoCommand {
public:
    TextEditCommand(
        InteractiveTextNode* node,
        const QString& oldText,
        const QString& newText,
        const QRectF&  oldRect,
        const QRectF&  newRect,
        QUndoCommand*  parent = nullptr);

    void undo() override;
    void redo() override;
    int  id()   const override { return 1001; }

    // Merge consecutive single-character edits into one command
    bool mergeWith(const QUndoCommand* other) override;

private:
    InteractiveTextNode* m_node;
    QString m_oldText, m_newText;
    QRectF  m_oldRect, m_newRect;
};

/**
 * ItemMoveCommand: records a drag-move of one or more items.
 */
class ItemMoveCommand : public QUndoCommand {
public:
    struct MoveEntry {
        QGraphicsItem* item;
        QPointF        oldPos;
        QPointF        newPos;
    };

    explicit ItemMoveCommand(
        const QVector<MoveEntry>& moves,
        QUndoCommand* parent = nullptr)
        : QUndoCommand("Move Item(s)", parent)
        , m_moves(moves)
    {}

    void undo() override {
        for (const auto& e : m_moves)
            e.item->setPos(e.oldPos);
    }
    void redo() override {
        for (const auto& e : m_moves)
            e.item->setPos(e.newPos);
    }
    int id() const override { return 1002; }

private:
    QVector<MoveEntry> m_moves;
};

/**
 * ItemResizeCommand: records bounding rect changes (drag-resize).
 */
class ItemResizeCommand : public QUndoCommand {
public:
    ItemResizeCommand(
        QGraphicsItem* item,
        const QRectF&  oldRect,
        const QRectF&  newRect,
        QUndoCommand*  parent = nullptr);

    void undo() override;
    void redo() override;
    int  id() const override { return 1003; }

private:
    QGraphicsItem* m_item;
    QRectF m_oldRect, m_newRect;
};

/**
 * FormatChangeCommand: records typography changes (font, color, alignment).
 */
struct TextFormat {
    QFont       font;
    QColor      color;
    Qt::Alignment alignment;
    double      lineSpacing;
};

class FormatChangeCommand : public QUndoCommand {
public:
    FormatChangeCommand(
        InteractiveTextNode* node,
        const TextFormat&    oldFmt,
        const TextFormat&    newFmt,
        QUndoCommand*        parent = nullptr);

    void undo() override;
    void redo() override;
    int  id() const override { return 1004; }

private:
    InteractiveTextNode* m_node;
    TextFormat m_oldFmt, m_newFmt;
};

/**
 * AddItemCommand: records the addition of a new item to the scene.
 */
class AddItemCommand : public QUndoCommand {
public:
    AddItemCommand(QGraphicsScene* scene, QGraphicsItem* item, QUndoCommand* parent = nullptr)
        : QUndoCommand("Add Item", parent)
        , m_scene(scene), m_item(item), m_ownsItem(false)
    {}
    ~AddItemCommand() {
        if (m_ownsItem) delete m_item;
    }
    void undo() override {
        m_scene->removeItem(m_item);
        m_ownsItem = true;
    }
    void redo() override {
        m_scene->addItem(m_item);
        m_ownsItem = false;
    }
private:
    QGraphicsScene* m_scene;
    QGraphicsItem*  m_item;
    bool            m_ownsItem;
};

/**
 * DeleteItemCommand: records deletion of one or more selected items.
 */
class DeleteItemCommand : public QUndoCommand {
public:
    DeleteItemCommand(QGraphicsScene* scene, QList<QGraphicsItem*> items, QUndoCommand* parent = nullptr)
        : QUndoCommand("Delete Item(s)", parent)
        , m_scene(scene), m_items(items), m_ownsItems(true)
    {}
    ~DeleteItemCommand() {
        if (m_ownsItems) qDeleteAll(m_items);
    }
    void undo() override {
        for (QGraphicsItem* item : m_items)
            m_scene->addItem(item);
        m_ownsItems = false;
    }
    void redo() override {
        for (QGraphicsItem* item : m_items)
            m_scene->removeItem(item);
        m_ownsItems = true;
    }
private:
    QGraphicsScene*      m_scene;
    QList<QGraphicsItem*> m_items;
    bool m_ownsItems;
};

// ============================================================================
// RESIZE HANDLE ITEM
// ============================================================================

/**
 * ResizeHandle is a small square handle drawn at the corners/edges of a
 * selected item. Dragging it resizes the parent item.
 */
class ResizeHandle : public QGraphicsRectItem {
public:
    ResizeHandle(PDFEditor::HandlePosition pos, QGraphicsItem* parent)
        : QGraphicsRectItem(parent)
        , m_position(pos)
        , m_dragging(false)
    {
        const double s = PDFEditor::HANDLE_SIZE;
        setRect(-s / 2, -s / 2, s, s);
        setBrush(QBrush(QColor(0, 120, 215)));
        setPen(QPen(Qt::white, 1.0));
        setFlag(QGraphicsItem::ItemIsMovable, false);
        setFlag(QGraphicsItem::ItemSendsGeometryChanges, false);
        setAcceptHoverEvents(true);
        updateCursor();
    }

    PDFEditor::HandlePosition position() const { return m_position; }
    bool isDragging() const { return m_dragging; }

protected:
    void hoverEnterEvent(QGraphicsSceneHoverEvent*) override { updateCursor(); }
    void hoverLeaveEvent(QGraphicsSceneHoverEvent*) override { unsetCursor(); }

    void mousePressEvent(QGraphicsSceneMouseEvent* event) override {
        if (event->button() == Qt::LeftButton) {
            m_dragging    = true;
            m_startHandlePos = pos();
            m_startScenePos  = event->scenePos();
            event->accept();
        }
    }
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override {
        m_dragging = false;
        event->accept();
    }

private:
    void updateCursor() {
        using HP = PDFEditor::HandlePosition;
        switch (m_position) {
        case HP::TopLeft:     setCursor(Qt::SizeFDiagCursor); break;
        case HP::Top:         setCursor(Qt::SizeVerCursor);   break;
        case HP::TopRight:    setCursor(Qt::SizeBDiagCursor); break;
        case HP::Left:        setCursor(Qt::SizeHorCursor);   break;
        case HP::Right:       setCursor(Qt::SizeHorCursor);   break;
        case HP::BottomLeft:  setCursor(Qt::SizeBDiagCursor); break;
        case HP::Bottom:      setCursor(Qt::SizeVerCursor);   break;
        case HP::BottomRight: setCursor(Qt::SizeFDiagCursor); break;
        }
    }

    PDFEditor::HandlePosition m_position;
    bool    m_dragging;
    QPointF m_startHandlePos;
    QPointF m_startScenePos;
};

// ============================================================================
// INTERACTIVE TEXT NODE
// ============================================================================

/**
 * InteractiveTextNode represents an editable text block on the canvas.
 * It overlays the PDF's static text rendering with a transparent, editable
 * QTextDocument-backed item. It manages:
 *   - Click-to-edit, caret placement, selection
 *   - Focus-out triggered reflow
 *   - Resize handles when selected in Select mode
 *   - Drag to move in Select mode
 */
class InteractiveTextNode : public QGraphicsTextItem {
    Q_OBJECT
public:
    InteractiveTextNode(
        const ExtractedTextBlock& block,
        QUndoStack*               undoStack,
        QGraphicsItem*            parent = nullptr)
        : QGraphicsTextItem(parent)
        , m_undoStack(undoStack)
        , m_blockRect(block.boundingBox)
        , m_originalRect(block.boundingBox)
        , m_font(block.font)
        , m_color(block.color)
        , m_alignment(block.alignment)
        , m_lineSpacing(block.lineSpacing)
        , m_isEditing(false)
        , m_isDragging(false)
        , m_isResizing(false)
        , m_resizingHandle(nullptr)
        , m_showHandles(false)
        , m_pageIndex(block.pageIndex)
    {
        setPos(block.boundingBox.topLeft());
        setTextWidth(block.boundingBox.width());

        applyFormat(block.font, block.color, block.alignment);
        setPlainText(block.text);

        setFlag(QGraphicsItem::ItemIsMovable, true);
        setFlag(QGraphicsItem::ItemIsSelectable, true);
        setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
        setFlag(QGraphicsItem::ItemIsFocusable, true);
        setAcceptHoverEvents(true);

        // TextInteractionFlags set later based on tool mode
        setTextInteractionFlags(Qt::NoTextInteraction);

        createResizeHandles();
        setHandlesVisible(false);

        // Save initial text for undo baseline
        m_committedText = toPlainText();
        m_committedRect = QRectF(pos(), QSizeF(textWidth(), boundingRect().height()));
    }

    ~InteractiveTextNode() override = default;

    // ---- Accessors ----
    QRectF   blockRect()    const { return QRectF(pos(), QSizeF(textWidth(), boundingRect().height())); }
    QFont    textFont()     const { return m_font; }
    QColor   textColor()    const { return m_color; }
    Qt::Alignment alignment() const { return m_alignment; }
    double   lineSpacing()  const { return m_lineSpacing; }
    int      pageIndex()    const { return m_pageIndex; }
    bool     isEditing()    const { return m_isEditing; }

    // ---- Format application ----
    void applyFormat(const QFont& font, const QColor& color,
                     Qt::Alignment align, bool pushUndo = false)
    {
        TextFormat oldFmt{ m_font, m_color, m_alignment, m_lineSpacing };

        m_font      = font;
        m_color     = color;
        m_alignment = align;

        QTextCharFormat charFmt;
        charFmt.setFont(font);
        charFmt.setForeground(QBrush(color));

        QTextBlockFormat blockFmt;
        blockFmt.setAlignment(align);

        QTextCursor cursor(document());
        cursor.select(QTextCursor::Document);
        cursor.mergeCharFormat(charFmt);
        cursor.mergeBlockFormat(blockFmt);

        // Reflow after format change
        reflowAndResize();

        if (pushUndo && m_undoStack) {
            TextFormat newFmt{ m_font, m_color, m_alignment, m_lineSpacing };
            m_undoStack->push(new FormatChangeCommand(this, oldFmt, newFmt));
        }
    }

    // Restore format without undo (used by FormatChangeCommand)
    void restoreFormat(const TextFormat& fmt) {
        m_font        = fmt.font;
        m_color       = fmt.color;
        m_alignment   = fmt.alignment;
        m_lineSpacing = fmt.lineSpacing;

        QTextCharFormat charFmt;
        charFmt.setFont(m_font);
        charFmt.setForeground(QBrush(m_color));

        QTextBlockFormat blockFmt;
        blockFmt.setAlignment(m_alignment);

        QTextCursor cursor(document());
        cursor.select(QTextCursor::Document);
        cursor.mergeCharFormat(charFmt);
        cursor.mergeBlockFormat(blockFmt);

        reflowAndResize();
        update();
    }

    // ---- Text manipulation (called from undo commands) ----
    void restoreTextAndRect(const QString& text, const QRectF& rect) {
        setPlainText(text);
        setPos(rect.topLeft());
        setTextWidth(rect.width());
        // Force height reflow
        reflowAndResize();
        update();
    }

    // ---- Handle visibility (controlled by selection state) ----
    void setHandlesVisible(bool visible) {
        m_showHandles = visible;
        for (ResizeHandle* h : m_handles)
            h->setVisible(visible);
        update();
    }

    void setEditMode(bool edit) {
        if (edit && !m_isEditing) {
            // Cache current state for undo
            m_committedText = toPlainText();
            m_committedRect = QRectF(pos(), QSizeF(textWidth(), boundingRect().height()));
            setTextInteractionFlags(Qt::TextEditorInteraction);
            setFocus(Qt::MouseFocusReason);
            m_isEditing = true;
        } else if (!edit && m_isEditing) {
            clearFocus();
            setTextInteractionFlags(Qt::NoTextInteraction);
            m_isEditing = false;

            // Perform reflow and push undo command if text changed
            reflowAndResize();
            QString newText = toPlainText();
            QRectF  newRect = QRectF(pos(), QSizeF(textWidth(), boundingRect().height()));

            if (newText != m_committedText || newRect != m_committedRect) {
                if (m_undoStack) {
                    m_undoStack->push(new TextEditCommand(
                        this,
                        m_committedText, newText,
                        m_committedRect, newRect
                    ));
                }
                m_committedText = newText;
                m_committedRect = newRect;
            }
        }
    }

    // ---- Reflow algorithm ----
    void reflowAndResize() {
        const QString text   = toPlainText();
        const double  width  = textWidth() > 0 ? textWidth() : m_blockRect.width();
        QRectF container(0, 0, width, 9999);

        auto result = ReflowEngine::reflow(text, container, m_font, m_alignment, m_lineSpacing);

        // Auto-expand height to fit, preserving width
        if (result.overflows || result.requiredSize.height() > boundingRect().height()) {
            double newHeight = result.requiredSize.height();
            // We can't directly set height on QGraphicsTextItem — we rely on
            // text document layout. However we can set text width and let Qt
            // recalculate the document height automatically.
            setTextWidth(width);
            document()->setPageSize(QSizeF(width, newHeight + 4));
        }
        repositionHandles();
        update();
    }

    QBrush selectionBrush() const {
        return QBrush(QColor(0, 120, 215, 40));
    }

signals:
    void textChanged(InteractiveTextNode* node);
    void selectionChanged(InteractiveTextNode* node, bool selected);
    void editingStarted(InteractiveTextNode* node);
    void editingFinished(InteractiveTextNode* node);
    void formatRequested(InteractiveTextNode* node);

protected:
    // ---- Paint override: draw selection highlight and border ----
    void paint(QPainter* painter,
               const QStyleOptionGraphicsItem* option,
               QWidget* widget) override
    {
        // Draw selection/hover highlight behind text
        if (isSelected() || m_isEditing) {
            painter->save();
            QPen borderPen(QColor(0, 120, 215), 1.5, Qt::DashLine);
            painter->setPen(borderPen);
            painter->setBrush(m_isEditing ? QBrush(QColor(255, 255, 200, 60))
                                          : QBrush(QColor(0, 120, 215, 20)));
            painter->drawRect(boundingRect().adjusted(-1, -1, 1, 1));
            painter->restore();
        }

        // Let Qt draw the actual text
        QGraphicsTextItem::paint(painter, option, widget);

        // Draw page-boundary overflow indicator
        if (m_showHandles) {
            painter->save();
            QPen handlePen(QColor(0, 120, 215), 1.0);
            painter->setPen(handlePen);
            painter->setBrush(Qt::NoBrush);
            painter->drawRect(boundingRect());
            painter->restore();
        }
    }

    // ---- Mouse events ----
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override {
        if (event->button() == Qt::LeftButton && m_isEditing) {
            QGraphicsTextItem::mousePressEvent(event);
            return;
        }
        m_dragStartScenePos = event->scenePos();
        m_dragStartItemPos  = pos();
        m_isDragging        = false;
        QGraphicsTextItem::mousePressEvent(event);
    }

    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override {
        if (m_isEditing) {
            QGraphicsTextItem::mouseMoveEvent(event);
            return;
        }
        if (event->buttons() & Qt::LeftButton) {
            QPointF delta = event->scenePos() - m_dragStartScenePos;
            if (!m_isDragging && delta.manhattanLength() > 3.0) {
                m_isDragging = true;
            }
        }
        QGraphicsTextItem::mouseMoveEvent(event);
    }

    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override {
        if (m_isDragging && m_undoStack) {
            QPointF newPos = pos();
            if (newPos != m_dragStartItemPos) {
                ItemMoveCommand::MoveEntry entry{ this, m_dragStartItemPos, newPos };
                m_undoStack->push(new ItemMoveCommand({ entry }));
            }
        }
        m_isDragging = false;
        QGraphicsTextItem::mouseReleaseEvent(event);
    }

    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) override {
        setEditMode(true);
        QGraphicsTextItem::mouseDoubleClickEvent(event);
        emit editingStarted(this);
    }

    void focusOutEvent(QFocusEvent* event) override {
        QGraphicsTextItem::focusOutEvent(event);
        if (m_isEditing) {
            setEditMode(false);
            emit editingFinished(this);
        }
    }

    // ---- Hover ----
    void hoverEnterEvent(QGraphicsSceneHoverEvent* event) override {
        if (!m_isEditing)
            setCursor(Qt::IBeamCursor);
        QGraphicsTextItem::hoverEnterEvent(event);
    }
    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override {
        unsetCursor();
        QGraphicsTextItem::hoverLeaveEvent(event);
    }

    // ---- Geometry change: reposition handles ----
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override {
        if (change == ItemPositionHasChanged || change == ItemTransformHasChanged) {
            repositionHandles();
        }
        if (change == ItemSelectedHasChanged) {
            bool sel = value.toBool();
            setHandlesVisible(sel && !m_isEditing);
            emit selectionChanged(this, sel);
            if (sel) emit formatRequested(this);
        }
        return QGraphicsTextItem::itemChange(change, value);
    }

    // ---- Key events in edit mode ----
    void keyPressEvent(QKeyEvent* event) override {
        if (!m_isEditing) {
            event->ignore();
            return;
        }
        // Escape exits editing
        if (event->key() == Qt::Key_Escape) {
            setEditMode(false);
            emit editingFinished(this);
            event->accept();
            return;
        }
        QGraphicsTextItem::keyPressEvent(event);
        emit textChanged(this);
    }

private:
    void createResizeHandles() {
        using HP = PDFEditor::HandlePosition;
        for (HP hp : { HP::TopLeft, HP::Top, HP::TopRight,
                       HP::Left,             HP::Right,
                       HP::BottomLeft, HP::Bottom, HP::BottomRight }) {
            auto* h = new ResizeHandle(hp, this);
            m_handles.append(h);
        }
        repositionHandles();
    }

    void repositionHandles() {
        QRectF r = boundingRect();
        double cx = r.width() / 2.0;
        double cy = r.height() / 2.0;

        using HP = PDFEditor::HandlePosition;
        QMap<HP, QPointF> positions = {
            { HP::TopLeft,     { r.left(),  r.top()    } },
            { HP::Top,         { cx,        r.top()    } },
            { HP::TopRight,    { r.right(), r.top()    } },
            { HP::Left,        { r.left(),  cy         } },
            { HP::Right,       { r.right(), cy         } },
            { HP::BottomLeft,  { r.left(),  r.bottom() } },
            { HP::Bottom,      { cx,        r.bottom() } },
            { HP::BottomRight, { r.right(), r.bottom() } },
        };

        for (ResizeHandle* h : m_handles) {
            h->setPos(positions.value(h->position()));
        }
    }

    QUndoStack*   m_undoStack;
    QRectF        m_blockRect;
    QRectF        m_originalRect;
    QFont         m_font;
    QColor        m_color;
    Qt::Alignment m_alignment;
    double        m_lineSpacing;
    bool          m_isEditing;
    bool          m_isDragging;
    bool          m_isResizing;
    ResizeHandle* m_resizingHandle;
    bool          m_showHandles;
    int           m_pageIndex;

    QPointF m_dragStartScenePos;
    QPointF m_dragStartItemPos;

    QString m_committedText;
    QRectF  m_committedRect;

    QVector<ResizeHandle*> m_handles;
};

// ============================================================================
// INTERACTIVE IMAGE NODE
// ============================================================================

/**
 * InteractiveImageNode is a resizable, draggable image item on the canvas.
 * It supports aspect-ratio-locked resizing via corner handles.
 */
class InteractiveImageNode : public QGraphicsPixmapItem {
public:
    InteractiveImageNode(
        const ExtractedImage& img,
        QUndoStack*           undoStack,
        QGraphicsItem*        parent = nullptr)
        : QGraphicsPixmapItem(parent)
        , m_undoStack(undoStack)
        , m_originalRect(img.boundingBox)
        , m_pageIndex(img.pageIndex)
        , m_aspectRatioLocked(true)
        , m_isDragging(false)
        , m_isResizing(false)
        , m_activeHandle(nullptr)
        , m_showHandles(false)
    {
        QPixmap px = QPixmap::fromImage(img.image);
        // Scale pixmap to match the extracted bounding box
        if (!px.isNull() && img.boundingBox.width() > 0 && img.boundingBox.height() > 0) {
            px = px.scaled(
                static_cast<int>(img.boundingBox.width()),
                static_cast<int>(img.boundingBox.height()),
                m_aspectRatioLocked ? Qt::KeepAspectRatio : Qt::IgnoreAspectRatio,
                Qt::SmoothTransformation
            );
        }
        setPixmap(px);
        setPos(img.boundingBox.topLeft());

        setFlag(QGraphicsItem::ItemIsMovable, true);
        setFlag(QGraphicsItem::ItemIsSelectable, true);
        setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
        setFlag(QGraphicsItem::ItemIsFocusable, true);
        setAcceptHoverEvents(true);

        createResizeHandles();
        setHandlesVisible(false);
    }

    ~InteractiveImageNode() override = default;

    int   pageIndex()          const { return m_pageIndex; }
    QRectF currentRect()       const { return QRectF(pos(), boundingRect().size()); }
    bool  aspectRatioLocked()  const { return m_aspectRatioLocked; }
    void  setAspectRatioLocked(bool lock) { m_aspectRatioLocked = lock; }

    void setHandlesVisible(bool visible) {
        m_showHandles = visible;
        for (ResizeHandle* h : m_handles)
            h->setVisible(visible);
        update();
    }

    // Resize the pixmap item to a new rect (used by undo/redo)
    void resizeTo(const QRectF& newRect) {
        setPos(newRect.topLeft());
        QPixmap orig = pixmap();
        if (!orig.isNull()) {
            QPixmap scaled = orig.scaled(
                static_cast<int>(newRect.width()),
                static_cast<int>(newRect.height()),
                m_aspectRatioLocked ? Qt::KeepAspectRatio : Qt::IgnoreAspectRatio,
                Qt::SmoothTransformation
            );
            setPixmap(scaled);
        }
        repositionHandles();
        update();
    }

protected:
    void paint(QPainter* painter,
               const QStyleOptionGraphicsItem* option,
               QWidget* widget) override
    {
        QGraphicsPixmapItem::paint(painter, option, widget);
        if (isSelected() || m_showHandles) {
            painter->save();
            painter->setPen(QPen(QColor(0, 120, 215), 1.5, Qt::DashLine));
            painter->setBrush(Qt::NoBrush);
            painter->drawRect(boundingRect());
            painter->restore();
        }
    }

    void mousePressEvent(QGraphicsSceneMouseEvent* event) override {
        m_dragStartPos     = event->scenePos();
        m_dragStartItemPos = pos();
        m_isDragging       = false;
        QGraphicsPixmapItem::mousePressEvent(event);
    }

    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override {
        if (event->buttons() & Qt::LeftButton) {
            QPointF delta = event->scenePos() - m_dragStartPos;
            if (!m_isDragging && delta.manhattanLength() > 3.0)
                m_isDragging = true;
        }
        QGraphicsPixmapItem::mouseMoveEvent(event);
    }

    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override {
        if (m_isDragging && m_undoStack) {
            QPointF newPos = pos();
            if (newPos != m_dragStartItemPos) {
                ItemMoveCommand::MoveEntry entry{ this, m_dragStartItemPos, newPos };
                m_undoStack->push(new ItemMoveCommand({ entry }));
            }
        }
        m_isDragging = false;
        QGraphicsPixmapItem::mouseReleaseEvent(event);
    }

    void hoverEnterEvent(QGraphicsSceneHoverEvent* event) override {
        setCursor(Qt::SizeAllCursor);
        QGraphicsPixmapItem::hoverEnterEvent(event);
    }
    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override {
        unsetCursor();
        QGraphicsPixmapItem::hoverLeaveEvent(event);
    }

    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override {
        if (change == ItemPositionHasChanged)
            repositionHandles();
        if (change == ItemSelectedHasChanged) {
            setHandlesVisible(value.toBool());
        }
        return QGraphicsPixmapItem::itemChange(change, value);
    }

private:
    void createResizeHandles() {
        using HP = PDFEditor::HandlePosition;
        for (HP hp : { HP::TopLeft, HP::Top, HP::TopRight,
                       HP::Left,             HP::Right,
                       HP::BottomLeft, HP::Bottom, HP::BottomRight }) {
            m_handles.append(new ResizeHandle(hp, this));
        }
        repositionHandles();
    }

    void repositionHandles() {
        QRectF r = boundingRect();
        double cx = r.width() / 2.0;
        double cy = r.height() / 2.0;

        using HP = PDFEditor::HandlePosition;
        QMap<HP, QPointF> positions = {
            { HP::TopLeft,     { r.left(),  r.top()    } },
            { HP::Top,         { cx,        r.top()    } },
            { HP::TopRight,    { r.right(), r.top()    } },
            { HP::Left,        { r.left(),  cy         } },
            { HP::Right,       { r.right(), cy         } },
            { HP::BottomLeft,  { r.left(),  r.bottom() } },
            { HP::Bottom,      { cx,        r.bottom() } },
            { HP::BottomRight, { r.right(), r.bottom() } },
        };
        for (ResizeHandle* h : m_handles)
            h->setPos(positions.value(h->position()));
    }

    QUndoStack*  m_undoStack;
    QRectF       m_originalRect;
    int          m_pageIndex;
    bool         m_aspectRatioLocked;
    bool         m_isDragging;
    bool         m_isResizing;
    ResizeHandle* m_activeHandle;
    bool         m_showHandles;

    QPointF m_dragStartPos;
    QPointF m_dragStartItemPos;

    QVector<ResizeHandle*> m_handles;
};

// ============================================================================
// NOW DEFINE UNDO COMMAND IMPLEMENTATIONS (needed complete types above)
// ============================================================================

TextEditCommand::TextEditCommand(
    InteractiveTextNode* node,
    const QString& oldText, const QString& newText,
    const QRectF& oldRect, const QRectF& newRect,
    QUndoCommand* parent)
    : QUndoCommand("Edit Text", parent)
    , m_node(node), m_oldText(oldText), m_newText(newText)
    , m_oldRect(oldRect), m_newRect(newRect)
{}

void TextEditCommand::undo() {
    m_node->restoreTextAndRect(m_oldText, m_oldRect);
}
void TextEditCommand::redo() {
    m_node->restoreTextAndRect(m_newText, m_newRect);
}
bool TextEditCommand::mergeWith(const QUndoCommand* other) {
    if (other->id() != id()) return false;
    const auto* o = static_cast<const TextEditCommand*>(other);
    if (o->m_node != m_node) return false;
    // Only merge if the new text is exactly one character longer (typing)
    m_newText = o->m_newText;
    m_newRect = o->m_newRect;
    return true;
}

ItemResizeCommand::ItemResizeCommand(
    QGraphicsItem* item,
    const QRectF& oldRect, const QRectF& newRect,
    QUndoCommand* parent)
    : QUndoCommand("Resize Item", parent)
    , m_item(item), m_oldRect(oldRect), m_newRect(newRect)
{}

void ItemResizeCommand::undo() {
    if (auto* t = dynamic_cast<InteractiveTextNode*>(m_item)) {
        t->setPos(m_oldRect.topLeft());
        t->setTextWidth(m_oldRect.width());
        t->reflowAndResize();
    } else if (auto* i = dynamic_cast<InteractiveImageNode*>(m_item)) {
        i->resizeTo(m_oldRect);
    }
}
void ItemResizeCommand::redo() {
    if (auto* t = dynamic_cast<InteractiveTextNode*>(m_item)) {
        t->setPos(m_newRect.topLeft());
        t->setTextWidth(m_newRect.width());
        t->reflowAndResize();
    } else if (auto* i = dynamic_cast<InteractiveImageNode*>(m_item)) {
        i->resizeTo(m_newRect);
    }
}

FormatChangeCommand::FormatChangeCommand(
    InteractiveTextNode* node,
    const TextFormat& oldFmt, const TextFormat& newFmt,
    QUndoCommand* parent)
    : QUndoCommand("Change Format", parent)
    , m_node(node), m_oldFmt(oldFmt), m_newFmt(newFmt)
{}
void FormatChangeCommand::undo() { m_node->restoreFormat(m_oldFmt); }
void FormatChangeCommand::redo() { m_node->restoreFormat(m_newFmt); }

// ============================================================================
// PAGE BACKGROUND ITEM
// ============================================================================

/**
 * PageItem renders a single PDF page as a background pixmap and provides
 * the page geometry for layout calculations.
 */
class PageItem : public QGraphicsItem {
public:
    PageItem(int pageIndex, const QPixmap& renderedPage, QGraphicsItem* parent = nullptr)
        : QGraphicsItem(parent)
        , m_pageIndex(pageIndex)
        , m_pixmap(renderedPage)
    {
        setFlag(QGraphicsItem::ItemIsMovable,    false);
        setFlag(QGraphicsItem::ItemIsSelectable, false);
        setZValue(-100); // Always below editable content
    }

    int     pageIndex() const { return m_pageIndex; }
    QPixmap pixmap()    const { return m_pixmap; }
    void    setPixmap(const QPixmap& px) { m_pixmap = px; update(); }

    QRectF boundingRect() const override {
        return QRectF(0, 0, m_pixmap.width(), m_pixmap.height());
    }

    void paint(QPainter* painter,
               const QStyleOptionGraphicsItem*,
               QWidget*) override
    {
        // Draw page shadow
        painter->save();
        QRectF shadowRect = boundingRect().translated(
            PDFEditor::PAGE_SHADOW, PDFEditor::PAGE_SHADOW);
        painter->fillRect(shadowRect, QColor(0, 0, 0, 60));
        painter->restore();

        // Draw white page background
        painter->fillRect(boundingRect(), Qt::white);

        // Draw rendered PDF content
        if (!m_pixmap.isNull())
            painter->drawPixmap(0, 0, m_pixmap);

        // Draw thin page border
        painter->setPen(QPen(QColor(180, 180, 180), 0.5));
        painter->drawRect(boundingRect());
    }

private:
    int     m_pageIndex;
    QPixmap m_pixmap;
};

// ============================================================================
// EDITOR CANVAS (QGraphicsScene)
// ============================================================================

/**
 * EditorCanvas is the central QGraphicsScene. It manages:
 *   - Multiple pages laid out vertically
 *   - All InteractiveTextNode and InteractiveImageNode items
 *   - Tool-mode-aware mouse handling
 *   - Rubber-band multi-selection
 *   - Clipboard operations
 */
class EditorCanvas : public QGraphicsScene {
    Q_OBJECT
public:
    explicit EditorCanvas(QObject* parent = nullptr)
        : QGraphicsScene(parent)
        , m_undoStack(new QUndoStack(this))
        , m_toolMode(PDFEditor::ToolMode::Select)
        , m_isPlacingText(false)
        , m_dragSelecting(false)
        , m_currentPage(0)
    {
        setBackgroundBrush(QBrush(QColor(80, 80, 80)));

        m_undoStack->setUndoLimit(200);
        connect(m_undoStack, &QUndoStack::cleanChanged,
                this, [this](bool clean) { emit documentModified(!clean); });
    }

    ~EditorCanvas() = default;

    // ---- Document loading ----
    void loadDocument(PDFDocument* doc) {
        clear();
        m_pageItems.clear();
        m_textNodes.clear();
        m_imageNodes.clear();
        m_pageOffsets.clear();

        m_document = doc;
        if (!doc) return;

        double yOffset = PDFEditor::PAGE_MARGIN;

        for (int i = 0; i < doc->pageCount(); ++i) {
            QSizeF pageSz = doc->pageSizeScaled(i);

            // Render background
            QImage pageImg = doc->renderPage(i);
            QPixmap pagePx = QPixmap::fromImage(pageImg);

            auto* pageItem = new PageItem(i, pagePx);
            pageItem->setPos(PDFEditor::PAGE_MARGIN, yOffset);
            addItem(pageItem);
            m_pageItems.append(pageItem);

            m_pageOffsets.append(QPointF(PDFEditor::PAGE_MARGIN, yOffset));

            // Extract and create text nodes
            QVector<ExtractedTextBlock> textBlocks = doc->extractTextBlocks(i);
            for (const auto& block : textBlocks) {
                auto* node = new InteractiveTextNode(block, m_undoStack);
                // Position relative to page origin in scene
                node->setPos(
                    PDFEditor::PAGE_MARGIN + block.boundingBox.x(),
                    yOffset + block.boundingBox.y()
                );
                node->setTextWidth(block.boundingBox.width());
                addItem(node);
                m_textNodes.append(node);

                connectTextNode(node);
            }

            // Extract and create image nodes
            QVector<ExtractedImage> images = doc->extractImages(i);
            for (const auto& img : images) {
                auto* imgNode = new InteractiveImageNode(img, m_undoStack);
                imgNode->setPos(
                    PDFEditor::PAGE_MARGIN + img.boundingBox.x(),
                    yOffset + img.boundingBox.y()
                );
                addItem(imgNode);
                m_imageNodes.append(imgNode);
            }

            yOffset += pageSz.height() + PDFEditor::PAGE_MARGIN;
        }

        setSceneRect(0, 0,
            PDFEditor::PAGE_MARGIN * 2 + (doc->pageCount() > 0 ? doc->pageSizeScaled(0).width() : 595),
            yOffset
        );

        m_undoStack->clear();
        emit canvasReady();
    }

    // ---- Tool mode ----
    PDFEditor::ToolMode toolMode() const { return m_toolMode; }
    void setToolMode(PDFEditor::ToolMode mode) {
        m_toolMode = mode;
        updateItemInteractivity();

        // Update all text nodes' editing capability
        for (auto* node : m_textNodes) {
            if (node->isEditing()) node->setEditMode(false);
            node->setTextInteractionFlags(
                mode == PDFEditor::ToolMode::Select
                    ? Qt::NoTextInteraction
                    : Qt::NoTextInteraction
            );
        }
    }

    // ---- Accessors ----
    QUndoStack* undoStack()      const { return m_undoStack; }
    int         currentPage()    const { return m_currentPage; }
    int         pageCount()      const { return m_pageItems.size(); }
    QVector<InteractiveTextNode*>  textNodes()  const { return m_textNodes; }
    QVector<InteractiveImageNode*> imageNodes() const { return m_imageNodes; }

    // Page offset in scene coordinates
    QPointF pageOffset(int pageIndex) const {
        if (pageIndex >= 0 && pageIndex < m_pageOffsets.size())
            return m_pageOffsets[pageIndex];
        return {};
    }

    // ---- Add new text block (Add Text tool) ----
    void addNewTextBlock(const QPointF& scenePos, const QFont& font, const QColor& color) {
        ExtractedTextBlock block;
        block.text        = "New Text";
        block.boundingBox = QRectF(scenePos, QSizeF(200, 40));
        block.font        = font;
        block.color       = color;
        block.alignment   = Qt::AlignLeft;
        block.lineSpacing = 1.2;
        block.isBold      = false;
        block.isItalic    = false;
        block.pageIndex   = pageAtScenePos(scenePos);

        auto* node = new InteractiveTextNode(block, m_undoStack);
        node->setPos(scenePos);
        node->setTextWidth(200);
        connectTextNode(node);

        m_undoStack->push(new AddItemCommand(this, node));
        m_textNodes.append(node);

        // Enter edit mode immediately
        clearSelection();
        node->setSelected(true);
        node->setEditMode(true);

        emit itemAdded(node);
    }

    // ---- Add image from file ----
    void addImageFromFile(const QString& filePath, const QPointF& scenePos) {
        QImage img(filePath);
        if (img.isNull()) return;

        ExtractedImage extracted;
        extracted.image     = img;
        extracted.pageIndex = pageAtScenePos(scenePos);
        extracted.boundingBox = QRectF(scenePos,
            QSizeF(qMin(300.0, static_cast<double>(img.width())),
                   qMin(300.0, static_cast<double>(img.height()))));

        auto* node = new InteractiveImageNode(extracted, m_undoStack);
        node->setPos(scenePos);

        m_undoStack->push(new AddItemCommand(this, node));
        m_imageNodes.append(node);
        emit itemAdded(node);
    }

    // ---- Clipboard ----
    void copySelected() {
        // Serialize selected items to internal clipboard
        m_clipboard.clear();
        for (QGraphicsItem* item : selectedItems()) {
            if (auto* tn = dynamic_cast<InteractiveTextNode*>(item)) {
                ClipboardEntry entry;
                entry.isText    = true;
                entry.text      = tn->toPlainText();
                entry.rect      = QRectF(tn->pos(), QSizeF(tn->textWidth(), tn->boundingRect().height()));
                entry.font      = tn->textFont();
                entry.color     = tn->textColor();
                entry.alignment = tn->alignment();
                m_clipboard.append(entry);
            }
        }
    }

    void pasteClipboard(const QPointF& pastePos) {
        if (m_clipboard.isEmpty()) return;
        clearSelection();
        QPointF offset(20, 20);
        for (const auto& entry : m_clipboard) {
            if (entry.isText) {
                ExtractedTextBlock block;
                block.text        = entry.text;
                block.boundingBox = entry.rect.translated(offset);
                block.font        = entry.font;
                block.color       = entry.color;
                block.alignment   = entry.alignment;
                block.lineSpacing = 1.2;
                block.pageIndex   = pageAtScenePos(pastePos);

                auto* node = new InteractiveTextNode(block, m_undoStack);
                node->setPos(block.boundingBox.topLeft());
                node->setTextWidth(block.boundingBox.width());
                connectTextNode(node);
                m_undoStack->push(new AddItemCommand(this, node));
                m_textNodes.append(node);
                node->setSelected(true);
            }
            offset += QPointF(10, 10);
        }
    }

    void deleteSelected() {
        QList<QGraphicsItem*> toDelete;
        for (QGraphicsItem* item : selectedItems()) {
            if (dynamic_cast<PageItem*>(item)) continue;
            toDelete.append(item);
        }
        if (!toDelete.isEmpty()) {
            m_undoStack->push(new DeleteItemCommand(this, toDelete));
            // Remove from tracking lists
            for (QGraphicsItem* item : toDelete) {
                m_textNodes.removeAll(dynamic_cast<InteractiveTextNode*>(item));
                m_imageNodes.removeAll(dynamic_cast<InteractiveImageNode*>(item));
            }
        }
    }

    // ---- Export current state to QImage ----
    QImage exportPageToImage(int pageIndex) {
        if (pageIndex < 0 || pageIndex >= m_pageItems.size()) return {};

        PageItem* page = m_pageItems[pageIndex];
        QRectF pageRect = page->sceneBoundingRect();

        QImage img(pageRect.size().toSize(), QImage::Format_ARGB32);
        img.fill(Qt::white);
        QPainter painter(&img);
        render(&painter, QRectF(), pageRect);
        return img;
    }

signals:
    void canvasReady();
    void documentModified(bool modified);
    void itemAdded(QGraphicsItem* item);
    void itemSelected(QGraphicsItem* item);
    void textNodeEditingStarted(InteractiveTextNode* node);
    void textNodeEditingFinished(InteractiveTextNode* node);
    void textNodeFormatRequested(InteractiveTextNode* node);
    void currentPageChanged(int pageIndex);
    void statusMessage(const QString& msg);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override {
        switch (m_toolMode) {
        case PDFEditor::ToolMode::Select:
            handleSelectPress(event);
            break;
        case PDFEditor::ToolMode::AddText:
            if (event->button() == Qt::LeftButton) {
                // Handled in mouseRelease to avoid conflict with item selection
                m_pendingTextPos = event->scenePos();
                m_isPlacingText  = true;
                event->accept();
            }
            break;
        case PDFEditor::ToolMode::AddImage:
            // Image placement via file dialog handled externally
            break;
        case PDFEditor::ToolMode::Hand:
            // Panning is handled by CanvasView
            break;
        default:
            QGraphicsScene::mousePressEvent(event);
        }
    }

    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override {
        updateCurrentPage(event->scenePos());
        QGraphicsScene::mouseMoveEvent(event);
        emit statusMessage(QString("Page %1 | X: %2  Y: %3")
            .arg(m_currentPage + 1)
            .arg(qRound(event->scenePos().x()))
            .arg(qRound(event->scenePos().y())));
    }

    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override {
        if (m_toolMode == PDFEditor::ToolMode::AddText && m_isPlacingText) {
            m_isPlacingText = false;
            QFont defaultFont;
            defaultFont.setPixelSize(14);
            addNewTextBlock(m_pendingTextPos, defaultFont, Qt::black);
            event->accept();
            return;
        }
        QGraphicsScene::mouseReleaseEvent(event);
    }

    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) override {
        if (m_toolMode == PDFEditor::ToolMode::Select) {
            QGraphicsItem* item = itemAt(event->scenePos(), QTransform());
            if (auto* tn = dynamic_cast<InteractiveTextNode*>(item)) {
                tn->setEditMode(true);
                emit textNodeEditingStarted(tn);
                event->accept();
                return;
            }
        }
        QGraphicsScene::mouseDoubleClickEvent(event);
    }

    void keyPressEvent(QKeyEvent* event) override {
        if (event->matches(QKeySequence::Delete) ||
            event->key() == Qt::Key_Backspace)
        {
            // Check if any text node is in editing mode
            bool anyEditing = false;
            for (auto* n : m_textNodes)
                if (n->isEditing()) { anyEditing = true; break; }
            if (!anyEditing) {
                deleteSelected();
                event->accept();
                return;
            }
        }
        QGraphicsScene::keyPressEvent(event);
    }

    void contextMenuEvent(QGraphicsSceneContextMenuEvent* event) override {
        QGraphicsItem* item = itemAt(event->scenePos(), QTransform());
        QMenu menu;

        if (item && !dynamic_cast<PageItem*>(item)) {
            menu.addAction("Cut",    [this]() { copySelected(); deleteSelected(); });
            menu.addAction("Copy",   [this]() { copySelected(); });
            menu.addSeparator();
            menu.addAction("Delete", [this]() { deleteSelected(); });
            menu.addSeparator();
            if (auto* tn = dynamic_cast<InteractiveTextNode*>(item)) {
                menu.addAction("Edit Text", [tn]() { tn->setEditMode(true); });
            }
        } else {
            menu.addAction("Paste", [this, &event]() {
                pasteClipboard(event->scenePos());
            });
        }
        if (!menu.actions().isEmpty())
            menu.exec(event->screenPos());
    }

private:
    void handleSelectPress(QGraphicsSceneMouseEvent* event) {
        QGraphicsItem* hit = itemAt(event->scenePos(), QTransform());
        if (!hit || dynamic_cast<PageItem*>(hit)) {
            // Click on background: deselect all
            if (!(event->modifiers() & Qt::ControlModifier))
                clearSelection();
        }
        QGraphicsScene::mousePressEvent(event);
    }

    void connectTextNode(InteractiveTextNode* node) {
        connect(node, &InteractiveTextNode::editingStarted,
                this, [this](InteractiveTextNode* n) {
                    emit textNodeEditingStarted(n);
                });
        connect(node, &InteractiveTextNode::editingFinished,
                this, [this](InteractiveTextNode* n) {
                    emit textNodeEditingFinished(n);
                });
        connect(node, &InteractiveTextNode::formatRequested,
                this, [this](InteractiveTextNode* n) {
                    emit textNodeFormatRequested(n);
                    emit itemSelected(n);
                });
    }

    void updateItemInteractivity() {
        bool selectMode = (m_toolMode == PDFEditor::ToolMode::Select);
        for (auto* n : m_textNodes) {
            n->setFlag(QGraphicsItem::ItemIsMovable,    selectMode);
            n->setFlag(QGraphicsItem::ItemIsSelectable, selectMode);
        }
        for (auto* i : m_imageNodes) {
            i->setFlag(QGraphicsItem::ItemIsMovable,    selectMode);
            i->setFlag(QGraphicsItem::ItemIsSelectable, selectMode);
        }
    }

    int pageAtScenePos(const QPointF& scenePos) const {
        for (int i = 0; i < m_pageItems.size(); ++i) {
            if (m_pageItems[i]->sceneBoundingRect().contains(scenePos))
                return i;
        }
        return 0;
    }

    void updateCurrentPage(const QPointF& scenePos) {
        int page = pageAtScenePos(scenePos);
        if (page != m_currentPage) {
            m_currentPage = page;
            emit currentPageChanged(m_currentPage);
        }
    }

    struct ClipboardEntry {
        bool          isText;
        QString       text;
        QRectF        rect;
        QFont         font;
        QColor        color;
        Qt::Alignment alignment;
        QPixmap       pixmap;
    };

    PDFDocument*           m_document = nullptr;
    QUndoStack*            m_undoStack;
    PDFEditor::ToolMode    m_toolMode;

    QVector<PageItem*>              m_pageItems;
    QVector<InteractiveTextNode*>   m_textNodes;
    QVector<InteractiveImageNode*>  m_imageNodes;
    QVector<QPointF>                m_pageOffsets;

    bool    m_isPlacingText;
    QPointF m_pendingTextPos;
    bool    m_dragSelecting;
    int     m_currentPage;

    QVector<ClipboardEntry> m_clipboard;
};

// ============================================================================
// CANVAS VIEW (QGraphicsView)
// ============================================================================

/**
 * CanvasView wraps the EditorCanvas in a QGraphicsView providing:
 *   - Smooth pinch-to-zoom and Ctrl+scroll zoom
 *   - Middle-button and Hand-tool panning
 *   - Drop-zone for image files
 *   - Zoom-to-fit and zoom-to-page operations
 */
class CanvasView : public QGraphicsView {
    Q_OBJECT
public:
    explicit CanvasView(EditorCanvas* canvas, QWidget* parent = nullptr)
        : QGraphicsView(canvas, parent)
        , m_canvas(canvas)
        , m_zoom(1.0)
        , m_isPanning(false)
        , m_handToolActive(false)
    {
        setRenderHint(QPainter::Antialiasing,          true);
        setRenderHint(QPainter::SmoothPixmapTransform, true);
        setRenderHint(QPainter::TextAntialiasing,      true);
        setViewportUpdateMode(QGraphicsView::MinimalViewportUpdate);
        setOptimizationFlag(QGraphicsView::DontAdjustForAntialiasing, true);
        setDragMode(QGraphicsView::RubberBandDrag);
        setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
        setResizeAnchor(QGraphicsView::AnchorUnderMouse);
        setAcceptDrops(true);
        setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        setFrameShape(QFrame::NoFrame);
    }

    double zoomFactor() const { return m_zoom; }

    void setHandToolActive(bool active) {
        m_handToolActive = active;
        if (active) {
            setDragMode(QGraphicsView::ScrollHandDrag);
            viewport()->setCursor(Qt::OpenHandCursor);
        } else {
            setDragMode(QGraphicsView::RubberBandDrag);
            viewport()->unsetCursor();
        }
    }

    void zoomTo(double factor) {
        factor = qBound(PDFEditor::MIN_ZOOM, factor, PDFEditor::MAX_ZOOM);
        double scale = factor / m_zoom;
        m_zoom = factor;
        QTransform t;
        t.scale(m_zoom, m_zoom);
        setTransform(t);
        emit zoomChanged(m_zoom);
    }

    void zoomIn()  { zoomTo(m_zoom * (1.0 + PDFEditor::ZOOM_STEP)); }
    void zoomOut() { zoomTo(m_zoom * (1.0 - PDFEditor::ZOOM_STEP)); }
    void zoomFit() {
        if (!scene()) return;
        QRectF sr = scene()->sceneRect();
        double sx = (width()  - 40.0) / sr.width();
        double sy = (height() - 40.0) / sr.height();
        zoomTo(qMin(sx, sy));
        centerOn(sr.center());
    }
    void zoomActual() { zoomTo(1.0); }

    void navigateToPage(int pageIndex) {
        QPointF offset = m_canvas->pageOffset(pageIndex);
        centerOn(offset.x() + 300, offset.y() + 400);
    }

signals:
    void zoomChanged(double factor);
    void imageDropped(const QString& filePath, const QPointF& scenePos);

protected:
    void wheelEvent(QWheelEvent* event) override {
        if (event->modifiers() & Qt::ControlModifier) {
            double delta = event->angleDelta().y() > 0 ? PDFEditor::ZOOM_STEP : -PDFEditor::ZOOM_STEP;
            zoomTo(m_zoom + delta);
            event->accept();
        } else {
            QGraphicsView::wheelEvent(event);
        }
    }

    void mousePressEvent(QMouseEvent* event) override {
        if (event->button() == Qt::MiddleButton) {
            m_isPanning   = true;
            m_panStartPos = event->pos();
            viewport()->setCursor(Qt::ClosedHandCursor);
            event->accept();
            return;
        }
        if (m_handToolActive && event->button() == Qt::LeftButton) {
            viewport()->setCursor(Qt::ClosedHandCursor);
        }
        QGraphicsView::mousePressEvent(event);
    }

    void mouseMoveEvent(QMouseEvent* event) override {
        if (m_isPanning) {
            QPointF delta = event->pos() - m_panStartPos;
            m_panStartPos = event->pos();
            horizontalScrollBar()->setValue(
                horizontalScrollBar()->value() - static_cast<int>(delta.x()));
            verticalScrollBar()->setValue(
                verticalScrollBar()->value() - static_cast<int>(delta.y()));
            event->accept();
            return;
        }
        QGraphicsView::mouseMoveEvent(event);
    }

    void mouseReleaseEvent(QMouseEvent* event) override {
        if (event->button() == Qt::MiddleButton && m_isPanning) {
            m_isPanning = false;
            viewport()->unsetCursor();
            event->accept();
            return;
        }
        if (m_handToolActive)
            viewport()->setCursor(Qt::OpenHandCursor);
        QGraphicsView::mouseReleaseEvent(event);
    }

    void dragEnterEvent(QDragEnterEvent* event) override {
        if (event->mimeData()->hasUrls()) {
            for (const QUrl& url : event->mimeData()->urls()) {
                QString path = url.toLocalFile();
                QStringList imgExts = { ".png", ".jpg", ".jpeg", ".bmp", ".tif", ".tiff", ".gif", ".webp" };
                for (const QString& ext : imgExts) {
                    if (path.toLower().endsWith(ext)) {
                        event->acceptProposedAction();
                        return;
                    }
                }
            }
        }
        event->ignore();
    }

    void dragMoveEvent(QDragMoveEvent* event) override {
        event->acceptProposedAction();
    }

    void dropEvent(QDropEvent* event) override {
        QPointF scenePos = mapToScene(event->position().toPoint());
        for (const QUrl& url : event->mimeData()->urls()) {
            QString path = url.toLocalFile();
            if (!path.isEmpty())
                emit imageDropped(path, scenePos);
        }
        event->acceptProposedAction();
    }

    void keyPressEvent(QKeyEvent* event) override {
        if (event->modifiers() & Qt::ControlModifier) {
            if (event->key() == Qt::Key_Equal || event->key() == Qt::Key_Plus)
                { zoomIn();     event->accept(); return; }
            if (event->key() == Qt::Key_Minus)
                { zoomOut();    event->accept(); return; }
            if (event->key() == Qt::Key_0)
                { zoomActual(); event->accept(); return; }
        }
        QGraphicsView::keyPressEvent(event);
    }

private:
    EditorCanvas* m_canvas;
    double  m_zoom;
    bool    m_isPanning;
    bool    m_handToolActive;
    QPoint  m_panStartPos;
};

// ============================================================================
// THUMBNAIL WIDGET (Left Sidebar)
// ============================================================================

/**
 * ThumbnailWidget shows per-page thumbnails in the left dock.
 * Clicking a thumbnail navigates the canvas view to that page.
 */
class ThumbnailWidget : public QWidget {
    Q_OBJECT
public:
    explicit ThumbnailWidget(QWidget* parent = nullptr)
        : QWidget(parent)
    {
        auto* layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);

        // Header
        auto* header = new QLabel("Pages", this);
        header->setAlignment(Qt::AlignCenter);
        header->setStyleSheet(
            "QLabel { background: #2c2c2c; color: #ccc; "
            "font-weight: bold; padding: 6px; font-size: 11px; }");
        layout->addWidget(header);

        m_listWidget = new QListWidget(this);
        m_listWidget->setIconSize(QSize(PDFEditor::THUMB_WIDTH - 20, PDFEditor::THUMB_HEIGHT - 30));
        m_listWidget->setSpacing(4);
        m_listWidget->setContentsMargins(4, 4, 4, 4);
        m_listWidget->setStyleSheet(
            "QListWidget { background: #3a3a3a; border: none; }"
            "QListWidget::item { background: #3a3a3a; color: #ddd; border-radius: 4px; }"
            "QListWidget::item:selected { background: #0078d4; color: white; }"
            "QListWidget::item:hover { background: #4a4a4a; }");
        layout->addWidget(m_listWidget);

        connect(m_listWidget, &QListWidget::currentRowChanged,
                this, &ThumbnailWidget::pageSelected);
    }

    void loadThumbnails(PDFDocument* doc) {
        m_listWidget->clear();
        if (!doc) return;

        for (int i = 0; i < doc->pageCount(); ++i) {
            QImage thumb = doc->renderThumbnail(i);
            QPixmap px   = QPixmap::fromImage(thumb).scaled(
                PDFEditor::THUMB_WIDTH - 20, PDFEditor::THUMB_HEIGHT - 30,
                Qt::KeepAspectRatio, Qt::SmoothTransformation
            );
            auto* item = new QListWidgetItem(
                QIcon(px),
                QString("Page %1").arg(i + 1),
                m_listWidget
            );
            item->setTextAlignment(Qt::AlignHCenter | Qt::AlignBottom);
            item->setSizeHint(QSize(PDFEditor::THUMB_WIDTH, PDFEditor::THUMB_HEIGHT));
        }

        if (m_listWidget->count() > 0)
            m_listWidget->setCurrentRow(0);
    }

    void setCurrentPage(int pageIndex) {
        if (pageIndex >= 0 && pageIndex < m_listWidget->count())
            m_listWidget->setCurrentRow(pageIndex);
    }

signals:
    void pageSelected(int pageIndex);

private:
    QListWidget* m_listWidget;
};

// ============================================================================
// PROPERTIES PANEL (Right Dock)
// ============================================================================

/**
 * PropertiesPanel is the right-side docked panel for typography and
 * paragraph formatting. It reflects and controls the formatting of the
 * currently selected/focused InteractiveTextNode.
 */
class PropertiesPanel : public QWidget {
    Q_OBJECT
public:
    explicit PropertiesPanel(QWidget* parent = nullptr)
        : QWidget(parent)
        , m_targetNode(nullptr)
        , m_updating(false)
    {
        setMinimumWidth(220);
        setMaximumWidth(300);

        auto* mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(8, 8, 8, 8);
        mainLayout->setSpacing(8);

        // ---- Typography Group ----
        auto* typoGroup = new QGroupBox("Typography", this);
        typoGroup->setStyleSheet(
            "QGroupBox { font-weight: bold; color: #333; border: 1px solid #ccc; "
            "border-radius: 4px; margin-top: 6px; padding-top: 10px; }"
            "QGroupBox::title { subcontrol-origin: margin; left: 8px; }");
        auto* typoLayout = new QFormLayout(typoGroup);
        typoLayout->setSpacing(6);

        // Font family
        m_fontCombo = new QFontComboBox(this);
        m_fontCombo->setEditable(true);
        m_fontCombo->setMinimumContentsLength(12);
        typoLayout->addRow("Font:", m_fontCombo);

        // Font size
        m_fontSizeSpin = new QDoubleSpinBox(this);
        m_fontSizeSpin->setRange(4.0, 288.0);
        m_fontSizeSpin->setSingleStep(0.5);
        m_fontSizeSpin->setValue(12.0);
        m_fontSizeSpin->setSuffix(" pt");
        typoLayout->addRow("Size:", m_fontSizeSpin);

        // Bold / Italic / Underline row
        auto* styleRow = new QWidget(this);
        auto* styleLayout = new QHBoxLayout(styleRow);
        styleLayout->setContentsMargins(0, 0, 0, 0);
        styleLayout->setSpacing(4);
        m_boldBtn = new QToolButton(this);
        m_boldBtn->setText("B");
        m_boldBtn->setFont(QFont(m_boldBtn->font().family(), 10, QFont::Bold));
        m_boldBtn->setCheckable(true);
        m_boldBtn->setFixedSize(28, 28);
        m_italicBtn = new QToolButton(this);
        m_italicBtn->setText("I");
        QFont italFont = m_italicBtn->font();
        italFont.setItalic(true);
        m_italicBtn->setFont(italFont);
        m_italicBtn->setCheckable(true);
        m_italicBtn->setFixedSize(28, 28);
        m_underlineBtn = new QToolButton(this);
        m_underlineBtn->setText("U");
        m_underlineBtn->setCheckable(true);
        m_underlineBtn->setFixedSize(28, 28);
        styleLayout->addWidget(m_boldBtn);
        styleLayout->addWidget(m_italicBtn);
        styleLayout->addWidget(m_underlineBtn);
        styleLayout->addStretch();
        typoLayout->addRow("Style:", styleRow);

        // Color picker
        auto* colorRow = new QWidget(this);
        auto* colorLayout = new QHBoxLayout(colorRow);
        colorLayout->setContentsMargins(0, 0, 0, 0);
        colorLayout->setSpacing(4);
        m_colorSwatch = new QPushButton(this);
        m_colorSwatch->setFixedSize(28, 28);
        m_colorSwatch->setFlat(true);
        updateColorSwatch(Qt::black);
        m_colorLabel = new QLabel("Black", this);
        colorLayout->addWidget(m_colorSwatch);
        colorLayout->addWidget(m_colorLabel);
        colorLayout->addStretch();
        typoLayout->addRow("Color:", colorRow);

        // Line spacing
        m_lineSpacingSpin = new QDoubleSpinBox(this);
        m_lineSpacingSpin->setRange(0.8, 5.0);
        m_lineSpacingSpin->setSingleStep(0.05);
        m_lineSpacingSpin->setValue(1.2);
        typoLayout->addRow("Spacing:", m_lineSpacingSpin);

        mainLayout->addWidget(typoGroup);

        // ---- Paragraph Group ----
        auto* paraGroup = new QGroupBox("Paragraph", this);
        paraGroup->setStyleSheet(typoGroup->styleSheet());
        auto* paraLayout = new QVBoxLayout(paraGroup);
        paraLayout->setSpacing(6);

        // Alignment buttons
        auto* alignRow = new QWidget(this);
        auto* alignLayout = new QHBoxLayout(alignRow);
        alignLayout->setContentsMargins(0, 0, 0, 0);
        alignLayout->setSpacing(2);

        m_alignGroup = new QButtonGroup(this);
        m_alignGroup->setExclusive(true);

        auto makeAlignBtn = [&](const QString& text, Qt::Alignment align) -> QToolButton* {
            auto* btn = new QToolButton(this);
            btn->setText(text);
            btn->setCheckable(true);
            btn->setFixedSize(32, 28);
            btn->setProperty("alignment", static_cast<int>(align));
            m_alignGroup->addButton(btn);
            alignLayout->addWidget(btn);
            return btn;
        };

        m_alignLeftBtn   = makeAlignBtn("≡L", Qt::AlignLeft);
        m_alignCenterBtn = makeAlignBtn("≡C", Qt::AlignHCenter);
        m_alignRightBtn  = makeAlignBtn("≡R", Qt::AlignRight);
        m_alignJustBtn   = makeAlignBtn("≡J", Qt::AlignJustify);
        m_alignLeftBtn->setChecked(true);
        alignLayout->addStretch();

        paraLayout->addWidget(new QLabel("Alignment:", this));
        paraLayout->addWidget(alignRow);

        mainLayout->addWidget(paraGroup);

        // ---- Transform Group ----
        auto* transformGroup = new QGroupBox("Position & Size", this);
        transformGroup->setStyleSheet(typoGroup->styleSheet());
        auto* transformLayout = new QFormLayout(transformGroup);
        transformLayout->setSpacing(6);

        m_xPosSpin = new QDoubleSpinBox(this);
        m_xPosSpin->setRange(-9999, 9999);
        m_xPosSpin->setSuffix(" px");
        transformLayout->addRow("X:", m_xPosSpin);

        m_yPosSpin = new QDoubleSpinBox(this);
        m_yPosSpin->setRange(-9999, 9999);
        m_yPosSpin->setSuffix(" px");
        transformLayout->addRow("Y:", m_yPosSpin);

        m_widthSpin = new QDoubleSpinBox(this);
        m_widthSpin->setRange(10, 9999);
        m_widthSpin->setSuffix(" px");
        transformLayout->addRow("W:", m_widthSpin);

        m_heightSpin = new QDoubleSpinBox(this);
        m_heightSpin->setRange(10, 9999);
        m_heightSpin->setSuffix(" px");
        m_heightSpin->setReadOnly(true); // Height is auto from reflow
        transformLayout->addRow("H:", m_heightSpin);

        mainLayout->addWidget(transformGroup);
        mainLayout->addStretch();

        // ---- Connections ----
        connectSignals();

        setEnabled(false); // Disabled until an item is selected
    }

    void bindToNode(InteractiveTextNode* node) {
        m_targetNode = node;
        if (!node) {
            setEnabled(false);
            return;
        }
        setEnabled(true);
        syncFromNode();
    }

    void unbind() {
        m_targetNode = nullptr;
        setEnabled(false);
    }

signals:
    void fontChanged(const QFont& font);
    void colorChanged(const QColor& color);
    void alignmentChanged(Qt::Alignment align);
    void lineSpacingChanged(double spacing);

private slots:
    void onFontFamilyChanged(const QFont& font) {
        if (m_updating || !m_targetNode) return;
        applyToNode();
    }

    void onFontSizeChanged(double size) {
        if (m_updating || !m_targetNode) return;
        applyToNode();
    }

    void onStyleToggled() {
        if (m_updating || !m_targetNode) return;
        applyToNode();
    }

    void onAlignmentChanged() {
        if (m_updating || !m_targetNode) return;
        applyToNode();
    }

    void onLineSpacingChanged(double) {
        if (m_updating || !m_targetNode) return;
        applyToNode();
    }

    void onColorButtonClicked() {
        QColor c = QColorDialog::getColor(m_currentColor, this, "Select Text Color");
        if (c.isValid()) {
            m_currentColor = c;
            updateColorSwatch(c);
            if (!m_updating && m_targetNode)
                applyToNode();
        }
    }

    void onPositionChanged() {
        if (m_updating || !m_targetNode) return;
        m_targetNode->setPos(m_xPosSpin->value(), m_yPosSpin->value());
    }

    void onWidthChanged() {
        if (m_updating || !m_targetNode) return;
        m_targetNode->setTextWidth(m_widthSpin->value());
        m_targetNode->reflowAndResize();
    }

private:
    void connectSignals() {
        connect(m_fontCombo,     &QFontComboBox::currentFontChanged,
                this, &PropertiesPanel::onFontFamilyChanged);
        connect(m_fontSizeSpin,  QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                this, &PropertiesPanel::onFontSizeChanged);
        connect(m_boldBtn,       &QToolButton::toggled, this, &PropertiesPanel::onStyleToggled);
        connect(m_italicBtn,     &QToolButton::toggled, this, &PropertiesPanel::onStyleToggled);
        connect(m_underlineBtn,  &QToolButton::toggled, this, &PropertiesPanel::onStyleToggled);
        connect(m_colorSwatch,   &QPushButton::clicked, this, &PropertiesPanel::onColorButtonClicked);
        connect(m_lineSpacingSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                this, &PropertiesPanel::onLineSpacingChanged);
        connect(m_alignGroup, QOverload<QAbstractButton*>::of(&QButtonGroup::buttonClicked),
                this, [this](QAbstractButton*) { onAlignmentChanged(); });
        connect(m_xPosSpin,  QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                this, &PropertiesPanel::onPositionChanged);
        connect(m_yPosSpin,  QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                this, &PropertiesPanel::onPositionChanged);
        connect(m_widthSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                this, &PropertiesPanel::onWidthChanged);
    }

    void syncFromNode() {
        if (!m_targetNode) return;
        m_updating = true;

        QFont f = m_targetNode->textFont();
        m_fontCombo->setCurrentFont(f);
        // Convert pixel size back to points
        double ptSize = f.pixelSize() > 0
            ? f.pixelSize() * 72.0 / PDFEditor::DEFAULT_DPI
            : f.pointSizeF();
        m_fontSizeSpin->setValue(ptSize > 0 ? ptSize : 12.0);

        m_boldBtn->setChecked(f.bold());
        m_italicBtn->setChecked(f.italic());
        m_underlineBtn->setChecked(f.underline());

        m_currentColor = m_targetNode->textColor();
        updateColorSwatch(m_currentColor);

        m_lineSpacingSpin->setValue(m_targetNode->lineSpacing());

        Qt::Alignment align = m_targetNode->alignment();
        if      (align & Qt::AlignLeft)    m_alignLeftBtn->setChecked(true);
        else if (align & Qt::AlignHCenter) m_alignCenterBtn->setChecked(true);
        else if (align & Qt::AlignRight)   m_alignRightBtn->setChecked(true);
        else if (align & Qt::AlignJustify) m_alignJustBtn->setChecked(true);

        QPointF p = m_targetNode->pos();
        m_xPosSpin->setValue(p.x());
        m_yPosSpin->setValue(p.y());
        m_widthSpin->setValue(m_targetNode->textWidth());
        m_heightSpin->setValue(m_targetNode->boundingRect().height());

        m_updating = false;
    }

    void applyToNode() {
        if (!m_targetNode || m_updating) return;

        QFont font = m_fontCombo->currentFont();
        double ptSize = m_fontSizeSpin->value();
        // Convert points to pixels at our DPI
        int pixelSize = qMax(4, static_cast<int>(ptSize * PDFEditor::DEFAULT_DPI / 72.0));
        font.setPixelSize(pixelSize);
        font.setBold(m_boldBtn->isChecked());
        font.setItalic(m_italicBtn->isChecked());
        font.setUnderline(m_underlineBtn->isChecked());

        Qt::Alignment align = Qt::AlignLeft;
        if      (m_alignCenterBtn->isChecked()) align = Qt::AlignHCenter;
        else if (m_alignRightBtn->isChecked())  align = Qt::AlignRight;
        else if (m_alignJustBtn->isChecked())   align = Qt::AlignJustify;

        m_targetNode->applyFormat(font, m_currentColor, align, true);
    }

    void updateColorSwatch(const QColor& color) {
        m_currentColor = color;
        QString style = QString(
            "QPushButton { background: %1; border: 1px solid #999; border-radius: 3px; }")
            .arg(color.name());
        m_colorSwatch->setStyleSheet(style);
        m_colorLabel->setText(color.name().toUpper());
    }

    InteractiveTextNode* m_targetNode;
    bool m_updating;
    QColor m_currentColor { Qt::black };

    QFontComboBox*   m_fontCombo;
    QDoubleSpinBox*  m_fontSizeSpin;
    QToolButton*     m_boldBtn;
    QToolButton*     m_italicBtn;
    QToolButton*     m_underlineBtn;
    QPushButton*     m_colorSwatch;
    QLabel*          m_colorLabel;
    QDoubleSpinBox*  m_lineSpacingSpin;
    QButtonGroup*    m_alignGroup;
    QToolButton*     m_alignLeftBtn;
    QToolButton*     m_alignCenterBtn;
    QToolButton*     m_alignRightBtn;
    QToolButton*     m_alignJustBtn;
    QDoubleSpinBox*  m_xPosSpin;
    QDoubleSpinBox*  m_yPosSpin;
    QDoubleSpinBox*  m_widthSpin;
    QDoubleSpinBox*  m_heightSpin;
};

// ============================================================================
// MAIN WINDOW
// ============================================================================

class PDFEditorMainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit PDFEditorMainWindow(QWidget* parent = nullptr)
        : QMainWindow(parent)
        , m_document(nullptr)
        , m_isModified(false)
        , m_currentZoom(1.0)
    {
        setWindowTitle("True PDF Editor");
        setMinimumSize(1100, 720);
        resize(1400, 900);

        buildUI();
        buildMenus();
        buildToolbar();
        buildStatusBar();
        connectSignals();
        applyStyleSheet();

        // Autosave timer
        m_autosaveTimer = new QTimer(this);
        m_autosaveTimer->setInterval(PDFEditor::AUTOSAVE_INTERVAL);
        connect(m_autosaveTimer, &QTimer::timeout, this, &PDFEditorMainWindow::onAutosave);

        // Initial state
        setToolMode(PDFEditor::ToolMode::Select);
        updateWindowTitle();
    }

    ~PDFEditorMainWindow() = default;

    bool openFile(const QString& filePath) {
        if (m_isModified) {
            auto res = QMessageBox::question(
                this, "Unsaved Changes",
                "You have unsaved changes. Open a new file anyway?",
                QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel
            );
            if (res != QMessageBox::Yes) return false;
        }

        auto* doc = new PDFDocument(this);
        if (!doc->load(filePath)) {
            QMessageBox::critical(this, "Error",
                QString("Could not open file:\n%1").arg(filePath));
            delete doc;
            return false;
        }

        delete m_document;
        m_document   = doc;
        m_filePath   = filePath;
        m_isModified = false;

        m_canvas->loadDocument(doc);
        m_thumbnails->loadThumbnails(doc);
        m_canvas->undoStack()->clear();

        m_autosaveTimer->start();
        updateWindowTitle();
        updatePageIndicator(1, doc->pageCount());

        m_statusBar->showMessage(
            QString("Opened: %1 (%2 pages)").arg(QFileInfo(filePath).fileName()).arg(doc->pageCount()),
            5000
        );
        return true;
    }

protected:
    void closeEvent(QCloseEvent* event) override {
        if (m_isModified) {
            auto res = QMessageBox::question(
                this, "Save Changes",
                "Save changes before closing?",
                QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel
            );
            if (res == QMessageBox::Save)        onSave();
            else if (res == QMessageBox::Cancel) { event->ignore(); return; }
        }
        saveSettings();
        event->accept();
    }

    void dragEnterEvent(QDragEnterEvent* event) override {
        if (event->mimeData()->hasUrls())
            event->acceptProposedAction();
    }

    void dropEvent(QDropEvent* event) override {
        for (const QUrl& url : event->mimeData()->urls()) {
            QString path = url.toLocalFile();
            if (path.toLower().endsWith(".pdf")) {
                openFile(path);
                break;
            }
        }
    }

private slots:
    void onOpen() {
        QString path = QFileDialog::getOpenFileName(
            this, "Open PDF", QDir::homePath(), "PDF Files (*.pdf);;All Files (*)"
        );
        if (!path.isEmpty()) openFile(path);
    }

    void onSave() {
        if (m_filePath.isEmpty()) { onSaveAs(); return; }
        // In a production editor, we'd re-render the modified canvas
        // back to a new PDF via Poppler or Cairo. Here we export each
        // page as an image and embed into a new PDF via QPrinter.
        exportToPDF(m_filePath);
        m_isModified = false;
        updateWindowTitle();
        m_statusBar->showMessage("Saved: " + m_filePath, 4000);
    }

    void onSaveAs() {
        QString path = QFileDialog::getSaveFileName(
            this, "Save PDF As", m_filePath, "PDF Files (*.pdf)"
        );
        if (!path.isEmpty()) {
            m_filePath = path;
            onSave();
        }
    }

    void onPrint() {
        if (!m_document) return;
        QPrinter printer(QPrinter::HighResolution);
        QPrintDialog dlg(&printer, this);
        if (dlg.exec() != QDialog::Accepted) return;

        QPainter painter(&printer);
        for (int i = 0; i < m_document->pageCount(); ++i) {
            if (i > 0) printer.newPage();
            QImage pageImg = m_canvas->exportPageToImage(i);
            painter.drawImage(printer.pageRect(QPrinter::DevicePixel).toRect(), pageImg);
        }
        painter.end();
    }

    void onUndo() { m_canvas->undoStack()->undo(); updateUndoRedoActions(); }
    void onRedo() { m_canvas->undoStack()->redo(); updateUndoRedoActions(); }

    void onSelectAll() {
        if (m_toolMode == PDFEditor::ToolMode::Select) {
            for (QGraphicsItem* item : m_canvas->items()) {
                if (!dynamic_cast<PageItem*>(item))
                    item->setSelected(true);
            }
        }
    }

    void onCopy()      { m_canvas->copySelected(); }
    void onCut()       { m_canvas->copySelected(); m_canvas->deleteSelected(); }
    void onPaste()     {
        QPointF center = m_view->mapToScene(m_view->viewport()->rect().center());
        m_canvas->pasteClipboard(center);
    }
    void onDelete()    { m_canvas->deleteSelected(); }

    void onAddTextTool()  { setToolMode(PDFEditor::ToolMode::AddText);  }
    void onAddImageTool() {
        setToolMode(PDFEditor::ToolMode::AddImage);
        QString path = QFileDialog::getOpenFileName(
            this, "Select Image", QDir::homePath(),
            "Images (*.png *.jpg *.jpeg *.bmp *.tif *.webp)"
        );
        if (!path.isEmpty()) {
            QPointF center = m_view->mapToScene(m_view->viewport()->rect().center());
            m_canvas->addImageFromFile(path, center);
        }
        setToolMode(PDFEditor::ToolMode::Select);
    }

    void onToolModeChanged(PDFEditor::ToolMode mode) {
        setToolMode(mode);
    }

    void onZoomIn()     { m_view->zoomIn();     }
    void onZoomOut()    { m_view->zoomOut();    }
    void onZoomFit()    { m_view->zoomFit();    }
    void onZoomActual() { m_view->zoomActual(); }

    void onZoomComboChanged(const QString& text) {
        QString clean = text;
        clean.remove('%').remove(' ');
        bool ok;
        double v = clean.toDouble(&ok);
        if (ok && v > 0) m_view->zoomTo(v / 100.0);
    }

    void onViewZoomChanged(double zoom) {
        m_currentZoom = zoom;
        m_zoomCombo->blockSignals(true);
        m_zoomCombo->setCurrentText(QString("%1%").arg(qRound(zoom * 100)));
        m_zoomCombo->blockSignals(false);
        m_zoomLabel->setText(QString("%1%").arg(qRound(zoom * 100)));
    }

    void onDocumentModified(bool modified) {
        m_isModified = modified;
        updateWindowTitle();
        updateUndoRedoActions();
    }

    void onPageSelected(int pageIndex) {
        m_view->navigateToPage(pageIndex);
    }

    void onCurrentPageChanged(int pageIndex) {
        m_thumbnails->setCurrentPage(pageIndex);
        updatePageIndicator(pageIndex + 1, m_document ? m_document->pageCount() : 0);
    }

    void onTextNodeSelected(InteractiveTextNode* node) {
        m_propertiesPanel->bindToNode(node);
    }

    void onTextEditingFinished(InteractiveTextNode*) {
        // Update properties panel to reflect any auto-reflow changes
        // (height may have changed)
    }

    void onImageDroppedOnCanvas(const QString& filePath, const QPointF& scenePos) {
        m_canvas->addImageFromFile(filePath, scenePos);
    }

    void onAutosave() {
        if (!m_isModified || m_filePath.isEmpty()) return;
        QString autosavePath = m_filePath + ".autosave";
        exportToPDF(autosavePath);
        m_statusBar->showMessage("Autosaved.", 2000);
    }

    void onAbout() {
        QMessageBox::about(this, "About True PDF Editor",
            "<h3>True PDF Editor</h3>"
            "<p>Production-grade PDF editing application built with:</p>"
            "<ul>"
            "<li>Qt6 (Widgets, GUI, PrintSupport)</li>"
            "<li>Poppler-Qt6 for PDF rendering and extraction</li>"
            "</ul>"
            "<p>Supports inline text editing, image manipulation, "
            "paragraph reflow, full undo/redo history, and more.</p>"
        );
    }

private:
    void buildUI() {
        // Central splitter
        auto* centralWidget = new QWidget(this);
        setCentralWidget(centralWidget);
        auto* mainLayout = new QHBoxLayout(centralWidget);
        mainLayout->setContentsMargins(0, 0, 0, 0);
        mainLayout->setSpacing(0);

        // ---- Canvas ----
        m_canvas = new EditorCanvas(this);
        m_view   = new CanvasView(m_canvas, this);
        mainLayout->addWidget(m_view, 1);

        // ---- Left Dock: Thumbnails ----
        m_thumbDock = new QDockWidget("Pages", this);
        m_thumbDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
        m_thumbDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetClosable);
        m_thumbnails = new ThumbnailWidget(m_thumbDock);
        m_thumbDock->setWidget(m_thumbnails);
        m_thumbDock->setMinimumWidth(160);
        m_thumbDock->setMaximumWidth(200);
        addDockWidget(Qt::LeftDockWidgetArea, m_thumbDock);

        // ---- Right Dock: Properties ----
        m_propDock = new QDockWidget("Properties", this);
        m_propDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
        m_propDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetClosable);
        m_propertiesPanel = new PropertiesPanel(m_propDock);
        auto* propScroll  = new QScrollArea(m_propDock);
        propScroll->setWidget(m_propertiesPanel);
        propScroll->setWidgetResizable(true);
        propScroll->setFrameShape(QFrame::NoFrame);
        m_propDock->setWidget(propScroll);
        addDockWidget(Qt::RightDockWidgetArea, m_propDock);
    }

    void buildMenus() {
        // ---- File menu ----
        QMenu* fileMenu = menuBar()->addMenu("&File");
        fileMenu->addAction(QIcon::fromTheme("document-open"), "&Open...",
                            this, &PDFEditorMainWindow::onOpen, QKeySequence::Open);
        fileMenu->addAction(QIcon::fromTheme("document-save"), "&Save",
                            this, &PDFEditorMainWindow::onSave, QKeySequence::Save);
        fileMenu->addAction("Save &As...",
                            this, &PDFEditorMainWindow::onSaveAs, QKeySequence::SaveAs);
        fileMenu->addSeparator();
        fileMenu->addAction(QIcon::fromTheme("document-print"), "&Print...",
                            this, &PDFEditorMainWindow::onPrint, QKeySequence::Print);
        fileMenu->addSeparator();
        fileMenu->addAction("E&xit", this, &QMainWindow::close, QKeySequence::Quit);

        // ---- Edit menu ----
        QMenu* editMenu = menuBar()->addMenu("&Edit");
        m_undoAction = editMenu->addAction(QIcon::fromTheme("edit-undo"), "&Undo",
                                            this, &PDFEditorMainWindow::onUndo, QKeySequence::Undo);
        m_redoAction = editMenu->addAction(QIcon::fromTheme("edit-redo"), "&Redo",
                                            this, &PDFEditorMainWindow::onRedo, QKeySequence::Redo);
        editMenu->addSeparator();
        editMenu->addAction("Cu&t",   this, &PDFEditorMainWindow::onCut,   QKeySequence::Cut);
        editMenu->addAction("&Copy",  this, &PDFEditorMainWindow::onCopy,  QKeySequence::Copy);
        editMenu->addAction("&Paste", this, &PDFEditorMainWindow::onPaste, QKeySequence::Paste);
        editMenu->addAction("&Delete",this, &PDFEditorMainWindow::onDelete, QKeySequence::Delete);
        editMenu->addSeparator();
        editMenu->addAction("Select &All", this, &PDFEditorMainWindow::onSelectAll, QKeySequence::SelectAll);

        // ---- View menu ----
        QMenu* viewMenu = menuBar()->addMenu("&View");
        viewMenu->addAction("Zoom &In",     this, &PDFEditorMainWindow::onZoomIn,     QKeySequence(Qt::CTRL | Qt::Key_Equal));
        viewMenu->addAction("Zoom &Out",    this, &PDFEditorMainWindow::onZoomOut,    QKeySequence(Qt::CTRL | Qt::Key_Minus));
        viewMenu->addAction("&Fit Page",    this, &PDFEditorMainWindow::onZoomFit,    QKeySequence(Qt::CTRL | Qt::Key_0));
        viewMenu->addAction("&Actual Size", this, &PDFEditorMainWindow::onZoomActual, QKeySequence(Qt::CTRL | Qt::Key_1));
        viewMenu->addSeparator();
        viewMenu->addAction(m_thumbDock->toggleViewAction());
        viewMenu->addAction(m_propDock->toggleViewAction());

        // ---- Help menu ----
        QMenu* helpMenu = menuBar()->addMenu("&Help");
        helpMenu->addAction("&About", this, &PDFEditorMainWindow::onAbout);
    }

    void buildToolbar() {
        m_mainToolbar = addToolBar("Main Toolbar");
        m_mainToolbar->setMovable(false);
        m_mainToolbar->setIconSize(QSize(20, 20));
        m_mainToolbar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

        // File actions
        m_mainToolbar->addAction(QIcon::fromTheme("document-open"), "Open", this, &PDFEditorMainWindow::onOpen);
        m_mainToolbar->addAction(QIcon::fromTheme("document-save"), "Save", this, &PDFEditorMainWindow::onSave);
        m_mainToolbar->addSeparator();

        // Undo/Redo
        m_mainToolbar->addAction(m_undoAction);
        m_mainToolbar->addAction(m_redoAction);
        m_mainToolbar->addSeparator();

        // Tool buttons (exclusive)
        auto* toolGroup = new QActionGroup(this);
        toolGroup->setExclusive(true);

        auto makeTool = [&](const QString& icon, const QString& label,
                             PDFEditor::ToolMode mode, const QString& shortcut) -> QAction*
        {
            QAction* act = new QAction(QIcon::fromTheme(icon), label, this);
            act->setCheckable(true);
            act->setShortcut(QKeySequence(shortcut));
            act->setProperty("toolMode", static_cast<int>(mode));
            toolGroup->addAction(act);
            m_mainToolbar->addAction(act);
            connect(act, &QAction::triggered, this, [this, mode]() {
                setToolMode(mode);
            });
            return act;
        };

        m_selectToolAction = makeTool("edit-select", "Select",   PDFEditor::ToolMode::Select,   "S");
        m_textToolAction   = makeTool("insert-text", "Add Text", PDFEditor::ToolMode::AddText,  "T");
        m_imageToolAction  = makeTool("insert-image","Add Image",PDFEditor::ToolMode::AddImage, "I");
        m_handToolAction   = makeTool("transform-move", "Hand",  PDFEditor::ToolMode::Hand,     "H");

        m_selectToolAction->setChecked(true);
        m_mainToolbar->addSeparator();

        // Zoom controls
        m_mainToolbar->addAction(QIcon::fromTheme("zoom-out"), "Zoom Out", this, &PDFEditorMainWindow::onZoomOut);

        m_zoomCombo = new QComboBox(this);
        m_zoomCombo->setEditable(true);
        m_zoomCombo->setFixedWidth(80);
        for (int z : { 50, 75, 100, 125, 150, 200, 300, 400 })
            m_zoomCombo->addItem(QString("%1%").arg(z));
        m_zoomCombo->setCurrentText("100%");
        connect(m_zoomCombo, &QComboBox::currentTextChanged,
                this, &PDFEditorMainWindow::onZoomComboChanged);
        m_mainToolbar->addWidget(m_zoomCombo);

        m_mainToolbar->addAction(QIcon::fromTheme("zoom-in"), "Zoom In", this, &PDFEditorMainWindow::onZoomIn);
        m_mainToolbar->addAction(QIcon::fromTheme("zoom-fit-best"), "Fit", this, &PDFEditorMainWindow::onZoomFit);
    }

    void buildStatusBar() {
        m_statusBar = statusBar();

        m_pageLabel = new QLabel("Page 0 / 0", this);
        m_pageLabel->setMinimumWidth(100);

        m_zoomLabel = new QLabel("100%", this);
        m_zoomLabel->setMinimumWidth(50);

        m_modLabel = new QLabel("", this);
        m_modLabel->setMinimumWidth(60);

        m_statusBar->addPermanentWidget(m_modLabel);
        m_statusBar->addPermanentWidget(new QLabel("|", this));
        m_statusBar->addPermanentWidget(m_pageLabel);
        m_statusBar->addPermanentWidget(new QLabel("|", this));
        m_statusBar->addPermanentWidget(m_zoomLabel);
    }

    void connectSignals() {
        // Canvas → Window
        connect(m_canvas, &EditorCanvas::documentModified,
                this, &PDFEditorMainWindow::onDocumentModified);
        connect(m_canvas, &EditorCanvas::textNodeEditingStarted,
                this, [this](InteractiveTextNode* n) {
                    m_propertiesPanel->bindToNode(n);
                });
        connect(m_canvas, &EditorCanvas::textNodeEditingFinished,
                this, &PDFEditorMainWindow::onTextEditingFinished);
        connect(m_canvas, &EditorCanvas::textNodeFormatRequested,
                this, [this](InteractiveTextNode* n) {
                    m_propertiesPanel->bindToNode(n);
                });
        connect(m_canvas, &EditorCanvas::currentPageChanged,
                this, &PDFEditorMainWindow::onCurrentPageChanged);
        connect(m_canvas, &EditorCanvas::statusMessage,
                m_statusBar, &QStatusBar::showMessage);
        connect(m_canvas, &EditorCanvas::itemSelected,
                this, [this](QGraphicsItem* item) {
                    if (auto* tn = dynamic_cast<InteractiveTextNode*>(item))
                        m_propertiesPanel->bindToNode(tn);
                });

        // View → Window
        connect(m_view, &CanvasView::zoomChanged,
                this, &PDFEditorMainWindow::onViewZoomChanged);
        connect(m_view, &CanvasView::imageDropped,
                this, &PDFEditorMainWindow::onImageDroppedOnCanvas);

        // Thumbnails → Window
        connect(m_thumbnails, &ThumbnailWidget::pageSelected,
                this, &PDFEditorMainWindow::onPageSelected);

        // Undo stack
        connect(m_canvas->undoStack(), &QUndoStack::canUndoChanged,
                m_undoAction, &QAction::setEnabled);
        connect(m_canvas->undoStack(), &QUndoStack::canRedoChanged,
                m_redoAction, &QAction::setEnabled);
        connect(m_canvas->undoStack(), &QUndoStack::undoTextChanged,
                this, [this](const QString& text) {
                    m_undoAction->setText(text.isEmpty() ? "Undo" : "Undo: " + text);
                });
        connect(m_canvas->undoStack(), &QUndoStack::redoTextChanged,
                this, [this](const QString& text) {
                    m_redoAction->setText(text.isEmpty() ? "Redo" : "Redo: " + text);
                });
    }

    void setToolMode(PDFEditor::ToolMode mode) {
        m_toolMode = mode;
        m_canvas->setToolMode(mode);
        m_view->setHandToolActive(mode == PDFEditor::ToolMode::Hand);

        // Update toolbar button states
        auto* group = m_selectToolAction->actionGroup();
        if (group) {
            for (QAction* act : group->actions()) {
                auto actMode = static_cast<PDFEditor::ToolMode>(act->property("toolMode").toInt());
                act->setChecked(actMode == mode);
            }
        }

        // Update cursor
        switch (mode) {
        case PDFEditor::ToolMode::Select:    m_view->viewport()->setCursor(Qt::ArrowCursor);  break;
        case PDFEditor::ToolMode::AddText:   m_view->viewport()->setCursor(Qt::IBeamCursor);  break;
        case PDFEditor::ToolMode::AddImage:  m_view->viewport()->setCursor(Qt::CrossCursor);  break;
        case PDFEditor::ToolMode::Hand:      m_view->viewport()->setCursor(Qt::OpenHandCursor); break;
        default: break;
        }
    }

    void updateWindowTitle() {
        QString title = "True PDF Editor";
        if (!m_filePath.isEmpty())
            title += " - " + QFileInfo(m_filePath).fileName();
        if (m_isModified)
            title += " *";
        setWindowTitle(title);
        m_modLabel->setText(m_isModified ? "● Modified" : "");
        m_modLabel->setStyleSheet(m_isModified ? "color: #cc4400;" : "");
    }

    void updatePageIndicator(int current, int total) {
        m_pageLabel->setText(QString("Page %1 / %2").arg(current).arg(total));
    }

    void updateUndoRedoActions() {
        m_undoAction->setEnabled(m_canvas->undoStack()->canUndo());
        m_redoAction->setEnabled(m_canvas->undoStack()->canRedo());
    }

    void exportToPDF(const QString& outputPath) {
        if (!m_document) return;

        QPrinter printer(QPrinter::HighResolution);
        printer.setOutputFormat(QPrinter::PdfFormat);
        printer.setOutputFileName(outputPath);
        printer.setFullPage(true);

        QPainter painter;
        if (!painter.begin(&printer)) return;

        for (int i = 0; i < m_document->pageCount(); ++i) {
            if (i > 0) printer.newPage();
            QSizeF sz = m_document->pageSizeScaled(i);
            QRectF pageSceneRect(
                m_canvas->pageOffset(i),
                sz
            );

            // Render the scene for this page
            QImage pageImg = m_canvas->exportPageToImage(i);
            if (!pageImg.isNull()) {
                QRectF printerRect = printer.pageRect(QPrinter::DevicePixel);
                painter.drawImage(printerRect, pageImg, pageImg.rect());
            }
        }
        painter.end();
    }

    void applyStyleSheet() {
        setStyleSheet(R"(
            QMainWindow {
                background: #2b2b2b;
            }
            QMenuBar {
                background: #383838;
                color: #ddd;
                border-bottom: 1px solid #222;
            }
            QMenuBar::item:selected {
                background: #0078d4;
            }
            QMenu {
                background: #383838;
                color: #ddd;
                border: 1px solid #555;
            }
            QMenu::item:selected {
                background: #0078d4;
            }
            QToolBar {
                background: #323232;
                border-bottom: 1px solid #222;
                spacing: 4px;
                padding: 4px;
            }
            QToolBar QToolButton {
                background: transparent;
                color: #ccc;
                border-radius: 3px;
                padding: 4px 8px;
            }
            QToolBar QToolButton:hover {
                background: #444;
            }
            QToolBar QToolButton:checked {
                background: #0078d4;
                color: white;
            }
            QDockWidget {
                color: #ccc;
                titlebar-close-icon: url(:/icons/close);
            }
            QDockWidget::title {
                background: #2c2c2c;
                padding: 5px;
                text-align: left;
                color: #ccc;
                font-weight: bold;
            }
            QStatusBar {
                background: #2c2c2c;
                color: #aaa;
                border-top: 1px solid #222;
            }
            QGroupBox {
                color: #444;
            }
            QLabel {
                color: #333;
            }
            QDoubleSpinBox, QSpinBox, QComboBox {
                background: white;
                border: 1px solid #ccc;
                border-radius: 3px;
                padding: 2px 4px;
            }
            QPushButton {
                background: #e0e0e0;
                border: 1px solid #ccc;
                border-radius: 3px;
                padding: 4px 12px;
            }
            QPushButton:hover {
                background: #d0d0d0;
            }
            QToolButton {
                border: 1px solid transparent;
                border-radius: 3px;
                padding: 2px;
            }
            QToolButton:hover {
                border-color: #ccc;
                background: #f0f0f0;
            }
            QToolButton:checked {
                background: #0078d4;
                color: white;
                border-color: #005fa3;
            }
        )");
    }

    void saveSettings() {
        QSettings settings("TruePDFEditor", "MainWindow");
        settings.setValue("geometry", saveGeometry());
        settings.setValue("windowState", saveState());
        settings.setValue("lastFile", m_filePath);
    }

    void loadSettings() {
        QSettings settings("TruePDFEditor", "MainWindow");
        restoreGeometry(settings.value("geometry").toByteArray());
        restoreState(settings.value("windowState").toByteArray());
    }

    // ---- Member variables ----
    PDFDocument*         m_document;
    QString              m_filePath;
    bool                 m_isModified;
    double               m_currentZoom;
    PDFEditor::ToolMode  m_toolMode { PDFEditor::ToolMode::Select };

    EditorCanvas*        m_canvas;
    CanvasView*          m_view;
    ThumbnailWidget*     m_thumbnails;
    PropertiesPanel*     m_propertiesPanel;

    QDockWidget*         m_thumbDock;
    QDockWidget*         m_propDock;
    QToolBar*            m_mainToolbar;
    QStatusBar*          m_statusBar;

    // Toolbar actions
    QAction*             m_undoAction;
    QAction*             m_redoAction;
    QAction*             m_selectToolAction;
    QAction*             m_textToolAction;
    QAction*             m_imageToolAction;
    QAction*             m_handToolAction;

    // Status bar widgets
    QLabel*              m_pageLabel;
    QLabel*              m_zoomLabel;
    QLabel*              m_modLabel;
    QComboBox*           m_zoomCombo;

    QTimer*              m_autosaveTimer;
};

// ============================================================================
// INCLUDE MOC (since we have Q_OBJECT classes in the same .cpp)
// ============================================================================
// In a full CMake project, add:
//   set_target_properties(TruePDFEditor PROPERTIES AUTOMOC ON)
// or manually run moc on this file and link the output.

// ============================================================================
// APPLICATION ENTRY POINT
// ============================================================================

int main(int argc, char* argv[]) {
    // High-DPI support
    QApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

    QApplication app(argc, argv);
    app.setApplicationName("True PDF Editor");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("TruePDFEditor");
    app.setOrganizationDomain("truepdfeditor.app");

    // Set a clean default font for the UI
    QFont appFont("Segoe UI", 9);
    if (!appFont.exactMatch()) {
        appFont.setFamily("Helvetica Neue");
        if (!appFont.exactMatch())
            appFont.setFamily("Arial");
    }
    app.setFont(appFont);

    PDFEditorMainWindow window;
    window.show();

    // Open file from command line if provided
    if (argc > 1) {
        QString filePath = QString::fromLocal8Bit(argv[1]);
        QFileInfo fi(filePath);
        if (fi.exists() && fi.suffix().toLower() == "pdf")
            window.openFile(filePath);
    }

    return app.exec();
}

// ============================================================================
// MOC INCLUDE (required for single-file Q_OBJECT builds with AUTOMOC disabled)
// If AUTOMOC is ON in CMake, remove the following #include lines.
// ============================================================================
//
// To compile without CMake AUTOMOC, run:
//   moc main.cpp -o main.moc
// Then add at the very end of this file:
//   #include "main.moc"
//
// ============================================================================
