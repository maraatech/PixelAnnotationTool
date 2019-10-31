#ifndef IMAGE_MASK_H
#define IMAGE_MASK_H

#include <QImage>
#include "boundingbox.h"
#include "utils.h"

struct ColorMask {
	QColor id;
	QColor color;
};

struct Mask {
    cv::Mat id;
    cv::Mat color;

    Mask clone();
};

struct MaskDiff {
    cv::Mat id;
    cv::Mat color;

    MaskDiff();
    MaskDiff(Mask src, Mask dest);

    Mask applyDiff(Mask mask);
    Mask removeDiff(Mask mask);
};

struct ImageMask {
    QImage id;
    QImage color;

    ImageMask();
    ImageMask(const QString &file, Id2Labels id_labels);
	ImageMask(const QString &file, Id2Labels id_labels, LabelInfo label);
    ImageMask(QSize s);

    Mask getMask();
    void setMask(Mask mask);

    int loadSmartMaskFile(const QString &file);
    int countInstances();
    ColorMask getSmartColorMask(ColorMask cm, int instance_num);
    void drawFillCircle(int x, int y, int pen_size, ColorMask cm);
    void drawPixel(int x, int y, ColorMask cm);
    void updateColor(const Id2Labels & labels);
    void exchangeLabel(int x, int y, const Id2Labels & id_labels, ColorMask cm);
    void fill(int x, int y, ColorMask cm, const Id2Labels & id_labels);
    void fill(int x, int y, ColorMask cm);
    void fillPolygon(cv::Mat& buffer, cv::Point point);
    cv::Scalar getColor(QColor& color);
    void createBoundingBox(int x, int y);
    void drawBoundingBox(int orig_x, int orig_y, int x, int y);
	
	void collapseMask(ImageMask mask);
};

class State{
    private:
        QImage display;
        ImageMask imsk;
        std::vector<BoundingBox> box_list;
};

class Action{
    public:
        explicit
        Action(int operation_code):nature(operation_code){
            //this->nature =  operation_code;
        }
        virtual void undo(State& state){};
    protected:
        const int nature;
        const static int BoundingBoxMoving =0;
        const static int BoundingBoxResizing =1;
        const static int Draw=2;
        const static int delete_box =3;
};

class DrawAction:Action{
    public:
        DrawAction(int operation_code,ImageMask imsk):Action(operation_code){
            this->imsk = imsk;
        }
        
        void undo(State& state){
            return;
        }
        
    private:
        ImageMask imsk;
};

#endif
