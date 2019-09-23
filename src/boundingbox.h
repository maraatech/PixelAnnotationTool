#ifndef BOUNDING_BOX_H
#define BOUNDING_BOX_H

#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <QImage>

typedef std::vector<cv::Point> PointList;

class BoundingBox{
    public:
        BoundingBox(cv::Point orig_point, cv::Point end_point, std::string name="");
        bool isWithinBoundingBox(int x, int y);
        bool isWithinBoundingBox(cv::Point p);
        bool isWithinResizingArea(cv::Point p);
        bool isWithinResizingArea(int x, int y);
        void move(int orig_x, int orig_y, int new_x, int new_y);
        void move(int x_diff, int y_diff);
        void select();
        void unselect();
        bool is_selected();
        void resize(int x_diff, int y_diff);
        PointList getFourCorners();
        int selectPoint(cv::Point p);
        bool compare(BoundingBox b);
        bool compare(std::string id);
        void draw(cv::Mat& image);
        void draw_marked(cv::Mat& image);
        cv::Point getMinMinPoint();
        cv::Point getMinMaxPoint();
        cv::Point getMaxMinPoint();
        cv::Point getMaxMaxPoint();
        int getWidth();
        int getHeight();
        void printBoxParam();
        std::string toXML();
        std::string getId(){return _id;};

        void setMaskColor(QColor color);
        QColor getMaskColor();
    private:
        std::string _object_name;
        int _min_x, _min_y, _max_x, _max_y;
        bool _is_selected =false;    
        std::string _id;
        int selected_corner =-1;
        QColor _mask_color = QColor(0, 0, 0);
};

#endif