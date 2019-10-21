#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include "boundingbox.h"
#include "utils.h"
#include <cmath>

#define MIN_DIST 15 

BoundingBox::BoundingBox(cv::Point orig_point, cv::Point end_point,std::string name){
    if(orig_point.x < end_point.x){
        this->_min_x = orig_point.x;
        this->_max_x = end_point.x;
    }else{
        this->_min_x = end_point.x;
        this->_max_x = orig_point.x;
    }
    if(orig_point.y < end_point.y){
        this->_min_y = orig_point.y;
        this->_max_y = end_point.y;
    }else{
        this->_min_y = end_point.y;
        this->_max_y = orig_point.y;
    }
    this->_id = generate_hex(6);
    this->_object_name = name;
}

bool BoundingBox::isWithinBoundingBox(int x, int y){
    if(x<_min_x || y<_min_y || x>_max_x || y>_max_y){
        return false;
    }
    return true;
}

bool BoundingBox::isWithinBoundingBox(cv::Point p){
    return isWithinBoundingBox(p.x, p.y);
}

bool BoundingBox::isWithinResizingArea(cv::Point p){
    return isWithinResizingArea(p.x, p.y);
}

void BoundingBox::move(int orig_x, int orig_y, int new_x, int new_y){
    int move_x = new_x - orig_x;
    int move_y = new_y - orig_y;
    move(move_x, move_y);
}

void BoundingBox::move(int x_diff, int y_diff){
    this->_min_x = this->_min_x+ x_diff;
    this->_max_x = this->_max_x+ x_diff;
    this->_min_y = this->_min_y+ y_diff;
    this->_max_y = this->_max_y+ y_diff;
}

//////////////////////////////////////////
/*       0                   1
 *        +-----------------+
 *        |                 |
 *        |                 |
 *        +-----------------+
 *       2                   3
 *
 * 0 = minmin
 * 1 = maxmin
 * 2 = minmax
 * 3 = maxmax
 *///////////////////////////////////////


void BoundingBox::resize(int x_diff, int y_diff){
    if(selected_corner ==0){
        this->_max_x = this->_max_x+x_diff;
        this->_max_y = this->_max_y+y_diff;
    }else if(selected_corner ==1){
        this->_min_x = this->_min_x+x_diff;
        this->_max_y = this->_max_y+y_diff;
    }else if(selected_corner ==2){
        this->_max_x = this->_max_x+x_diff;
        this->_min_y = this->_min_y+y_diff;
    }else if(selected_corner ==3){
        this->_min_x = this->_min_x+x_diff;
        this->_min_y = this->_min_y+y_diff;
    }
}

void BoundingBox::select(){
    std::cout<<"selected"<<std::endl;
    printBoxParam();
    this->_is_selected = true;
}

void BoundingBox::unselect(){
    std::cout<<"unselected"<<std::endl;
    this->_is_selected = false;
}

float distance(int x1, int y1, int x2, int y2){
    float x_diff = (float)(x1 - x2);
    float y_diff = (float)(y1 - y2);
    float dist = sqrt(pow(x_diff,2)+ pow(y_diff,2));
    std::cout<<"distance is"<<dist<<" from "<<cv::Point(x1,y1)<<cv::Point(x2,y2)<<std::endl;
    return dist;
}

float distance(cv::Point p1, cv::Point p2){
    return distance(p1.x, p1.y, p2.x, p2.y);
}

PointList BoundingBox::getFourCorners(){
    PointList list;
    list.push_back(cv::Point(this->_min_x, this->_min_y));
    list.push_back(cv::Point(this->_max_x, this->_min_y));
    list.push_back(cv::Point(this->_min_x, this->_max_y));
    list.push_back(cv::Point(this->_max_x, this->_max_y));
    return list;
}

int BoundingBox::selectPoint(cv::Point p){
    PointList list = getFourCorners();
    for(int i =0; i<list.size();i++){
        cv::Point corner_p = list[i];
        if(distance(corner_p,p)<MIN_DIST){
            selected_corner = 3- i;
            return selected_corner;
        }
    }
    return -1;
}

bool BoundingBox::isWithinResizingArea(int x, int y){
    return selectPoint(cv::Point(x,y))>-1;
}

bool BoundingBox::compare(BoundingBox b){
    return this->_id.compare(b._id) ==0;
}

bool BoundingBox::compare(std::string id){
    return this->_id.compare(id) ==0;
}

void BoundingBox::draw(cv::Mat& image){
    rectangle(image,cv::Point(this->_min_x,this->_min_y),cv::Point(this->_max_x,this->_max_y),cv::Scalar(255,0,0),2);
}

void BoundingBox::draw_marked(cv::Mat& image){
    rectangle(image,cv::Point(this->_min_x,this->_min_y),cv::Point(this->_max_x,this->_max_y),cv::Scalar(0,0,255),2);
}

bool BoundingBox::is_selected(){
    return _is_selected;
}

int BoundingBox::getHeight(){
    selected_corner = -1;
    return _max_y-_min_y;
}

int BoundingBox::getWidth(){
    return _max_x-_min_x;
}

cv::Point BoundingBox::getMinMinPoint(){
    return cv::Point(_min_x, _min_y);
}

cv::Point BoundingBox::getMinMaxPoint(){
    return cv::Point(_min_x, _max_y);
}

cv::Point BoundingBox::getMaxMinPoint(){
    return cv::Point(_max_x, _min_y);
}

cv::Point BoundingBox::getMaxMaxPoint(){
    return cv::Point(_max_x, _max_y);
}

QColor BoundingBox::getMaskColor() {
    return _mask_color;
}

void BoundingBox::setMaskColor(QColor color) {
    _mask_color = color;
}

void BoundingBox::printBoxParam(){
    std::cout<<"box parameters are"<<cv::Point(_min_x,_min_y)<<cv::Point(_max_x,_max_y)<<" width:"<<getWidth()<<" height"<<getHeight()<<std::endl;
}

std::string BoundingBox::toXML(){
    return "\t<object>\n\
            <name>"+_object_name+"</name>\n\
            <pose>Unspecified</pose>\n\
            <truncated>0</truncated>\n\
            <difficult>0</difficult>\n\
            <bndbox>\n\
            <xmin>"+std::to_string(_min_x)+"</xmin>\n\
            <ymin>"+std::to_string(_min_y)+"</ymin>\n\
            <xmax>"+std::to_string(_max_x)+"</xmax>\n\
            <ymax>"+std::to_string(_max_y)+"</ymax>\n\
            <mask_r>"+std::to_string(_mask_color.red())+"</mask_r>\n\
            <mask_g>"+std::to_string(_mask_color.green())+"</mask_g>\n\
            <mask_b>"+std::to_string(_mask_color.blue())+"</mask_b>\n\
            </bndbox>\n\
            \t</object>\n";
}
