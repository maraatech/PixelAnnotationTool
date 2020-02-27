#ifndef IMAGE_CANVAS_H
#define IMAGE_CANVAS_H

#include "utils.h"
#include "image_mask.h"
#include "boundingbox.h"
#include "operations.h"

#include <QLabel>
#include <QPen>
#include <QScrollArea>
#include <stack>

class MainWindow;

class OperationManager;

struct WindowState {
	ImageMask mask;
	std::vector<BoundingBox> bbox_list;
};

class ImageCanvas : public QLabel {
    Q_OBJECT

public:

    ImageCanvas(MainWindow *ui);
    ~ImageCanvas();

    void setId(int id);
    void setImageMask(const ImageMask & mask);
    void setSmartImageMask(const ImageMask & smart_mask);
    void setBoxList(const std::vector<BoundingBox> &b_list) { box_list = b_list; }
    ImageMask getImageMask() const { return _mask; }
    ImageMask getSmartImageMask() const { return _mask; }
    QImage getImage() const { return _image; }
    std::vector<BoundingBox>& getBoxList() { return box_list; }

    void refresh();
    void updateMaskColor(const Id2Labels & labels) { _mask.updateColor(labels); }
    void load(const QString &filename);
    QScrollArea * getScrollParent() const { return _scroll_parent; }
    bool isNotSaved();
    void redrawBoundingBox(int except_index =-1);
    int getSelectedBox();
    void reset(int operation=DRAW_MODE);
    void drawMarkedBoundingBox(BoundingBox b);
    void drawBoundingBox(BoundingBox b);
    std::string getObjectString();

    void regenerate();

    ColorMask getColorMask(bool smart = false);

protected:
    void mouseMoveEvent(QMouseEvent * event) override;
    void mousePressEvent(QMouseEvent * event) override;
    void keyPressEvent(QKeyEvent * event) override;
    void wheelEvent(QWheelEvent * event) override;
    void mouseReleaseEvent(QMouseEvent * event) override;
    void paintEvent(QPaintEvent *event) override;

public slots :
    void scaleChanged(double);
    void alphaChanged(double);
    void setSizePen(int);
    void clearMask();
    void save();
    void smartMask();
    void undo();
    void redo();

private:
    MainWindow *_ui;

    void _initPixmap();
    void _drawFillCircle(QMouseEvent * e);
    void _fill(QMouseEvent * e);
    void _startMarkingBoundingBox(QMouseEvent *e);
    void _drawBoundingBox(QMouseEvent *e);
    cv::Point getXYonImage(QMouseEvent *e);
    cv::Point getXYonImage(int x_gui, int y_gui);
    BoundingBox parseXML(QString file_name);

    std::string createXML(BoundingBox bbox);



    void delete_layer(int index);

    void delete_last_layer();
    void restore_last_layer();

    void clear_box_selection();


    QScrollArea     *_scroll_parent    ;
    double           _scale            ;
    double           _alpha            ;
    QImage           _image            ;
    QImage           _orig_image       ;
    QImage           _buffer_image     ;
    ImageMask        _smart_mask       ;
    ImageMask        _mask             ;
    QPoint           _mouse_pos        ;
    QString          _img_file         ;
    QString          _mask_file        ;
    QString          _smart_mask_file  ;
    QString          _annotation_file  ;
    ColorMask        _color            ;
    int              _instance_num     ;
    int              _pen_size         ;
    bool             _button_is_pressed;
    int start_x;
    int start_y;

    std::vector<BoundingBox> box_list;
    std::vector<ImageMask> mask_history;
    std::stack<ImageMask> delete_stack;
    ImageMask _top_mask;
    ImageMask _prev_mask;
    
	std::vector<WindowState> state_history;
	std::stack<WindowState> delete_state_history;

    int _cid = 0; // Default to first label

    int _continous_click = 0;
    
    const int FILL_IN_MODIFIER = Qt::ShiftModifier;
    const int BBOX_MODIFIER = Qt::ControlModifier;
    
    int _operation_mode;
    static const int DRAW_MODE=0;
    static const int BOX_SELECTED =1;
    static const int BOX_MOVING =2;
    static const int BOX_RESIZING =3;
    static const int BOX_UNSELECTING = 4;
    static const int BOX_CREATING = 5;
};



#endif //IMAGE_CANVAS_H
