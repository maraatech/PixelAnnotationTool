
#include "image_canvas.h"
#include "main_window.h"

#include <QtDebug>
#include <QtWidgets>
#include <fstream>
#include <QtXml>

#define MAX_PATH_LENGTH 260
#define MAX_INSTANCE_COUNT 10000

ImageCanvas::ImageCanvas(MainWindow *ui) :
    QLabel() ,
    _ui(ui){

    _scroll_parent = new QScrollArea(ui);
    setParent(_scroll_parent);
    resize(800,600);
    _scale = _ui->spinbox_scale->value();
    _alpha = _ui->spinbox_alpha->value();
    _pen_size = _ui->spinbox_pen_size->value();
    _initPixmap();
    setScaledContents(true);
    setMouseTracking(true);
    _button_is_pressed = false;

    _scroll_parent->setBackgroundRole(QPalette::Dark);
    _scroll_parent->setWidget(this);
    _operation_mode = DRAW_MODE;
    _instance_num = 0;

    _op_manager = new OperationManager(this);
}

ImageCanvas::~ImageCanvas() {
    _scroll_parent->deleteLater();
}

bool ImageCanvas::isNotSaved() { 
    return _op_manager->num_ops() > 0;
}

void ImageCanvas::_initPixmap() {
    QPixmap newPixmap = QPixmap(width(), height());
    newPixmap.fill(Qt::white);
    QPainter painter(&newPixmap);
    const QPixmap * p = pixmap();
    if (p != NULL)
        painter.drawPixmap(0, 0, *pixmap());
    painter.end();
    setPixmap(newPixmap);
}

void ImageCanvas::loadImage(const QString &filename) {
    if (!_image.isNull() )
        save();

    _img_file = filename;
    QFileInfo file(_img_file);

    if (!file.exists()) return;

    _image = mat2QImage(cv::imread(_img_file.toStdString()));
    _orig_image = _image.copy();

    _mask_file = file.dir().absolutePath()+ "/" + file.baseName() + "_color_mask.png";
    _annotation_file = file.dir().absolutePath()+ "/xml/" + file.baseName() + "_bbox.xml";

    if (QFile(_mask_file).exists()) {
        _mask = ImageMask(_mask_file,_ui->id_labels);
        _top_mask = ImageMask(_image.size());
        _ui->checkbox_manuel_mask->setChecked(true);
    } else {
        clearMask();
    }

    parseXML(_annotation_file);
    _ui->undo_action->setEnabled(true);
    _ui->redo_action->setEnabled(true);

    setPixmap(QPixmap::fromImage(_image));
    resize(_scale *_image.size());
    redrawBoundingBox();
}

void ImageCanvas::parseXML(QString file_name){
    // Create a document to write XML
    QDomDocument document;
    // Open a file for reading
    QFile file(file_name);
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        std::cout << "cant find file."<<file_name.toStdString()<<std::endl;
        return;
    }else{
        // loading
        if(!document.setContent(&file))
        {
            std::cout << "Failed to load the file for reading.";
            return;
        }
        file.close();
    }
    // Getting root element
    QDomElement root = document.firstChildElement();
    QDomNodeList nodes = root.elementsByTagName("object");
    for(int i = 0; i < nodes.count(); i++) {
        QDomElement boundbox_node = nodes.at(i).toElement().elementsByTagName("bndbox").at(0).toElement();
        QString name = boundbox_node.elementsByTagName("name").at(0).firstChild().toText().data();
        int min_x = boundbox_node.elementsByTagName("xmin").at(0).firstChild().toText().data().toInt();
        int max_x = boundbox_node.elementsByTagName("xmax").at(0).firstChild().toText().data().toInt();
        int min_y = boundbox_node.elementsByTagName("ymin").at(0).firstChild().toText().data().toInt();
        int max_y = boundbox_node.elementsByTagName("ymax").at(0).firstChild().toText().data().toInt();

        auto bbox = BoundingBox(cv::Point(min_x,min_y),cv::Point(max_x, max_y),name.toStdString());

        box_list.push_back(bbox);
    }
    return;
}

void ImageCanvas::smartMask() {

    int index = getSelectedBox();

    if (index != -1)
    {
        //~~~ Modify existing layer ~~~//
        // 1. Modify bounding box
        BoundingBox bbox = findBoundingBox(qImage2Mat(mask_history[index].id), _ui->id_labels);
        box_list[index] = bbox;
    }
    else
    {
        //~~~ Add new layer ~~~//

        // 1. Push onto stack

        _prev_mask.setMask(_mask.getMask().clone());
        mask_history.push_back(_top_mask);
        // 2. Add bounding box
        BoundingBox bbox = findBoundingBox(qImage2Mat(_top_mask.id), _ui->id_labels);
        box_list.push_back(bbox);
    }


    redrawBoundingBox();
    _top_mask = ImageMask(_image.size());
    return;
}

void ImageCanvas::load(const QString &filename) {
    if (!_image.isNull() )
        save();

    _img_file = QDir(_ui->curr_open_dir).filePath(filename);
    QFileInfo file(_img_file);
    QString base = file.baseName();
    if (!file.exists())
    {
        return;
    }

    // Load image


    _image = mat2QImage(cv::imread(file.absoluteFilePath().toStdString()));
    _orig_image = _image.copy();

    // Load masks and xml

    bool existingData = false;

    for (int file_counter = 0; file_counter < MAX_INSTANCE_COUNT; file_counter++)
    {
        char mask_filename[MAX_PATH_LENGTH] = {0};
        char xml_filename[MAX_PATH_LENGTH] = {0};
        sprintf(mask_filename, "%s_%04d.png", file.baseName().toStdString().c_str(), file_counter);
        sprintf(xml_filename, "%s_%04d.xml", file.baseName().toStdString().c_str(), file_counter);

        QString xml_abs_path = QDir(_ui->xml_annotations).filePath(xml_filename);
        QString mask_abs_path = QDir(_ui->mask_annotations).filePath(mask_filename);
        if (!QFile().exists(xml_abs_path) || !QFile().exists(mask_abs_path))
        {
            break;
        }

        existingData = true;

        ImageMask mask(mask_abs_path, _ui->id_labels);
        mask_history.push_back(mask);
        parseXML(xml_abs_path);
    }

    if (!existingData)
    {
        clearMask();
    }
    else
    {
        regenerate();
    }

    setPixmap(QPixmap::fromImage(_image));
    resize(_scale *_image.size());
    redrawBoundingBox();

//    _mask_file = file.dir().absolutePath()+ "/" + file.baseName() + "_color_mask.png";
//    _annotation_file = file.dir().absolutePath()+ "/xml/" + file.baseName() + "_bbox.xml";

//    if (QFile(_mask_file).exists()) {
//        _mask = ImageMask(_mask_file,_ui->id_labels);
//        _top_mask = ImageMask(_image.size());
//        _ui->checkbox_manuel_mask->setChecked(true);
//    } else {
//        clearMask();
//    }

//    parseXML(_annotation_file);
//    _ui->undo_action->setEnabled(true);
//    _ui->redo_action->setEnabled(true);

//    setPixmap(QPixmap::fromImage(_image));
//    resize(_scale *_image.size());
//    redrawBoundingBox();



    // Load Bounding Boxes
}

void ImageCanvas::save() {
    QFileInfo file(_img_file);


    // Save each mask as seperate .png denoted by (Image ID)_XXXX.png
    // Save XML as (Image ID)_XXXX.xml
    for (int i = 0; i < mask_history.size(); i++)
    {
        auto mask = mask_history[i];
        auto bbox = box_list[i];

        char mask_filename[MAX_PATH_LENGTH] = {0};
        char bounding_box_filename[MAX_PATH_LENGTH] = {0};
        sprintf(mask_filename, "%s_%04d.png", file.baseName().toStdString().c_str(), i);
        sprintf(bounding_box_filename, "%s_%04d.xml", file.baseName().toStdString().c_str(), i);

        QString mask_abs_path = QDir(_ui->mask_annotations).filePath(mask_filename);
        QString xml_abs_path = QDir(_ui->xml_annotations).filePath(bounding_box_filename);

        mask.id.save(mask_abs_path);

        auto xml_string = createXML(bbox);

        std::string file_name = xml_abs_path.toStdString();
        std::ofstream out(file_name);
        out << xml_string;
        out.close();
    }

    // Copy input images to jpgs
    QString source_abs_path = QDir(_ui->source_images).filePath(file.completeBaseName());
    if (QFile::exists(source_abs_path))
    {
        QFile::remove(source_abs_path);
    }

    QFile::copy(file.absoluteFilePath(), source_abs_path);

    _ui->setStarAtNameOfTab(false);
}


void ImageCanvas::saveMask() {
    if (isFullZero(_mask.id))
        return;


    _mask.id.save(_mask_file);
    QFileInfo file(_img_file);

    QString color_file = file.dir().absolutePath() + "/" + file.baseName() + "_color_mask.png";
    _mask.color.save(color_file);
    _smart_mask.color.save(_smart_mask_file);
    saveAnnotation();
    _ui->setStarAtNameOfTab(false);
}

std::string ImageCanvas::createXML(BoundingBox bbox)
{
    QFileInfo file(_img_file);
    std::string xml_text = "";

    xml_text += "<annotation>\n";

    xml_text += "\t<folder>jpgs</folder>\n";

    xml_text += "\t<source>\n";
    xml_text += "\t\t<database>OXIIIT Custom</database>\n";
    xml_text += "\t\t<annotation>OXIIIT Bespoke</annotation>\n";
    xml_text += "\t\t<image>Self</image>\n";
    xml_text += "\t</source>\n";

    xml_text += "\t<size>\n";
    xml_text += "\t\t<width>" + std::to_string(width()) + "</width>\n";
    xml_text += "\t\t<height>" + std::to_string(height()) + "</height>\n";
    xml_text += "\t\t<depth>3</depth>\n";
    xml_text += "\t</size>\n";

    xml_text += "\t<segmented>0</segmented>\n";

    xml_text += "\t<object>\n";
    xml_text += "\t\t<name>" + bbox.getObjectName() + "</name>\n";
    xml_text += "\t\t<pose>Frontal</pose>\n";
    xml_text += "\t\t<truncated>0</truncated>\n";
    xml_text += "\t\t<occluded>0</occluded>\n";

    xml_text += "\t\t<bndbox>\n";
    xml_text += "\t\t\t<xmin>" + std::to_string(0) + "</xmin>\n";
    xml_text += "\t\t\t<ymin>" + std::to_string(0) + "</ymin>\n";
    xml_text += "\t\t\t<xmax>" + std::to_string(0) + "</xmax>\n";
    xml_text += "\t\t\t<ymax>" + std::to_string(0) + "</ymax>\n";
    xml_text += "\t\t</bndbox>\n";

    xml_text += "\t\t<difficult>0</difficult>";

    xml_text += "\t</object>\n";

    xml_text += "</annotation>";

    return xml_text;
}

void ImageCanvas::saveAnnotation(){
    QFileInfo file(_img_file);
    //create text
    std::string text ="<annotation>\n\
            <folder>0</folder>\n\
            <filename>"+ file.baseName().toStdString()+"</filename>\n\
            <path>"+file.dir().absolutePath().toStdString() + "/" + file.baseName().toStdString()+"</path>\n\
            <source>\n\
            \t<database>Unknown</database>\n\
            </source>\n\
            <size>\n\
            \t<width>"+std::to_string(width())+"</width>\n\
            \t<height>"+std::to_string(height())+"</height>\n\
            \t<depth>3</depth>\n\
            </size>\n\
            <segmented>0</segmented>\n";
            for(BoundingBox b:box_list){
            text+=b.toXML();
}
            text = text+ "\n</annotation>";
    std::string file_name = file.dir().absolutePath().toStdString() + "/xml/"+file.baseName().toStdString()+"_bbox.xml";
    std::ofstream out(file_name);
    out << text;
    out.close();
    std::cout<<"saving xml"<<file_name<<std::endl;
}

void ImageCanvas::scaleChanged(double scale) {
    _scale  = scale ;
    resize(_scale * _image.size());
    repaint();
}

void ImageCanvas::alphaChanged(double alpha) {
    _alpha = alpha;
    repaint();
}

void ImageCanvas::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, false);
    QRect rect = painter.viewport();
    QSize size = _scale * _image.size();
    if (size != _image.size()) {
        rect.size().scale(size, Qt::KeepAspectRatio);
        painter.setViewport(rect.x(), rect.y(), size.width(), size.height());
        painter.setWindow(pixmap()->rect());
    }
    painter.drawImage(QPoint(0, 0), _image);
    painter.setOpacity(_alpha);

    if (!_mask.id.isNull() && _ui->checkbox_manuel_mask->isChecked()) {
        painter.drawImage(QPoint(0, 0), _mask.color);
    }

    if (_mouse_pos.x() > 10 && _mouse_pos.y() > 10 &&
            _mouse_pos.x() <= QLabel::size().width()-10 &&
            _mouse_pos.y() <= QLabel::size().height()-10) {
        painter.setBrush(QBrush(_color.color));
        painter.setPen(QPen(QBrush(_color.color), 1.0));
        painter.drawEllipse(_mouse_pos.x() / _scale - _pen_size / 2, _mouse_pos.y() / _scale - _pen_size / 2, _pen_size, _pen_size);
        painter.end();
    }
}

cv::Point ImageCanvas::getXYonImage(QMouseEvent *e){
    return getXYonImage(e->x(), e->y());
}

cv::Point ImageCanvas::getXYonImage(int x_gui, int y_gui){
    int x =0;
    int y =0;
    if (_pen_size > 0) {
        x = x_gui / _scale - _pen_size / 2;
        y = y_gui / _scale - _pen_size / 2;
    } else {
        x = (x_gui+0.5) / _scale ;
        y = (y_gui+0.5) / _scale ;
    }
    return cv::Point(x,y);
}


int ImageCanvas::getSelectedBox(){
    for(int i =0;i<box_list.size();i++){
        if(box_list[i].is_selected()){
            return i;
        }
    }
    return -1;
}

//remember e is xy of gui
//xy should be converted of the image

void ImageCanvas::drawBoundingBox(BoundingBox b){
    cv::Mat _image_mat = qImage2Mat(_image);
    b.draw(_image_mat);
    _image = mat2QImage(_image_mat);
    update();
}

void ImageCanvas::drawMarkedBoundingBox(BoundingBox b){
    cv::Mat _image_mat = qImage2Mat(_image);
    b.draw_marked(_image_mat);
    _image = mat2QImage(_image_mat);
    update();
}

void ImageCanvas::mousePressEvent(QMouseEvent * e) {
    setFocus();
    cv::Point p = getXYonImage(e);
    std::cout<<"clicked"<<p<<" operation:"<<_operation_mode<<" modifiers:"<<e->modifiers()<<std::endl;
    if (e->button() == Qt::LeftButton) {
        _button_is_pressed = true;

        int idx = getSelectedBox();
        if (idx != -1) {
            std::cout<<"selected index"<<idx<<std::endl;
            redrawBoundingBox();
        }
        if (_operation_mode == DRAW_MODE) {
            if(FILL_IN_MODIFIER==e->modifiers()) {
                // FILL in Operation
                _fill(e);
                return;
            } else if (BBOX_MODIFIER==e->modifiers() && _cid != -1) {
                // Select BOX for modification
                //check if its within range
                std::vector<int> select_indexes = {};
                for(int i =0; i< box_list.size();i++){
                    if(box_list[i].isWithinBoundingBox(getXYonImage(e))) {
                        select_indexes.push_back(i);
                    }
                }

                // Found boxes in range
                if (!select_indexes.empty()) {
                    int index = select_indexes[_continous_click % select_indexes.size()];
                    box_list[index].select();
                    drawMarkedBoundingBox(box_list[index]);
                    _continous_click++;
                    return;

                }

                // Did not find boxes in range
                _continous_click = 0;
                clear_box_selection();
                return;
            } else {
                _drawFillCircle(e);
                return;
            }
        }
    } else if(e->button() == Qt::RightButton) {
        // Noop for right button at the moment
        
    }
}

void ImageCanvas::mouseMoveEvent(QMouseEvent * e) {
    _mouse_pos.setX(e->x());
    _mouse_pos.setY(e->y());
    cv::Point cur_pt = getXYonImage(e);
    if (_button_is_pressed) {
        _drawFillCircle(e);
    }
    update();
}

void ImageCanvas::mouseReleaseEvent(QMouseEvent * e) {
    if(e->button() == Qt::LeftButton) {
        std::cout<<"mouse released"<<getXYonImage(e)<<std::endl;
        _button_is_pressed = false;
        if(_operation_mode == DRAW_MODE){
            _op_manager->save_draw();
        }
        _ui->setStarAtNameOfTab(true);
    }

    if (e->button() == Qt::RightButton) { // selection of label
        QColor color = _mask.id.pixel(_mouse_pos / _scale);
        const LabelInfo * label = _ui->id_labels[color.red()];

        if(label->item != NULL) {
            emit(_ui->list_label->currentItemChanged(label->item, NULL));
        }

        refresh();
    }

    if (e->button() == Qt::MiddleButton)
    {
        cv::Point p = getXYonImage(e);
        int x = p.x;
        int y = p.y;

        _mask.exchangeLabel(x, y, _ui->id_labels, _color);
        update();
    }
}


void ImageCanvas::reset(int operation){
    start_x =-1;
    start_y =-1;
    _operation_mode = operation;
    redrawBoundingBox();
    update();
}

void ImageCanvas::setSizePen(int pen_size) {
    _pen_size = pen_size;
}

void ImageCanvas::_drawFillCircle(QMouseEvent * e) {
    cv::Point p = getXYonImage(e);
    int x = p.x;
    int y = p.y;

    int idx = getSelectedBox();

    ColorMask cm = getColorMask();
    if (_pen_size > 0) {
        _mask.drawFillCircle(x, y, _pen_size, cm);
        if (idx != -1) {
            mask_history[idx].drawFillCircle(x, y, _pen_size, cm);
        } else {
            _top_mask.drawFillCircle(x, y, _pen_size, cm);
        }

    } else {
        _mask.drawPixel(x, y, cm);
        if (idx != -1) {
            mask_history[idx].drawPixel(x, y, cm);
        } else {
            _top_mask.drawPixel(x, y, cm);
        }
    }

    update();
}

ColorMask ImageCanvas::getColorMask(bool smart) {

    ColorMask cm;
    cm.color = _ui->id_labels[_cid]->color;
    cm.id = QColor(_cid, _cid, _cid);

    return cm;
}

void ImageCanvas::_fill(QMouseEvent *e){
    cv::Point p = getXYonImage(e);
    int x = p.x;
    int y = p.y;

    int idx = getSelectedBox();

    _mask.fill(x, y, getColorMask(false),_ui->id_labels);

    if (idx != -1)
    {
        mask_history[idx].fill(x, y, getColorMask(false),_ui->id_labels);
    }
    else
    {
        _top_mask.fill(x, y, getColorMask(false),_ui->id_labels);
    }
}

void ImageCanvas::_startMarkingBoundingBox(QMouseEvent *e){
    cv::Point p = getXYonImage(e);
    this->start_x = p.x;
    this->start_y = p.y;
    //store display to buffer
    _buffer_image = _image.copy();
}

void ImageCanvas::_drawBoundingBox(QMouseEvent *e){
    //reset buffer to display on move
    _image = _buffer_image.copy();
    cv::Mat _image_mat = qImage2Mat(_image);
    rectangle(_image_mat,cv::Point(start_x,start_y),getXYonImage(e),cv::Scalar(255,0,0),2);
    _image = mat2QImage(_image_mat);
}

//from scratch
void ImageCanvas::redrawBoundingBox(int except_index){
    _image = _orig_image.copy();
    cv::Mat _image_mat = qImage2Mat(_image);
    for(int i =0; i < this->box_list.size(); i++){
        if(i == except_index){
            continue;
        }
        if (box_list[i].is_selected())
        {
            box_list[i].draw_marked(_image_mat);
        } else {
            box_list[i].draw(_image_mat);
        }


    }
    _image = mat2QImage(_image_mat);
}

void ImageCanvas::clearMask() {
    _mask = ImageMask(_image.size());
    _smart_mask = ImageMask(_image.size());
    _top_mask = ImageMask(_image.size());
    repaint();
}

void ImageCanvas::wheelEvent(QWheelEvent * event) {
    int delta = event->delta() > 0 ? 1 : -1;
    if (Qt::ShiftModifier == event->modifiers()) {
        _scroll_parent->verticalScrollBar()->setEnabled(false);
        int value = _ui->spinbox_pen_size->value() + delta * _ui->spinbox_pen_size->singleStep();
        _ui->spinbox_pen_size->setValue(value);
        emit(_ui->spinbox_pen_size->valueChanged(value));
        setSizePen(value);
        repaint();
    } else if (Qt::ControlModifier == event->modifiers()) {
        _scroll_parent->verticalScrollBar()->setEnabled(false);
        double value = _ui->spinbox_scale->value() + delta * _ui->spinbox_scale->singleStep();
        value = std::min<double>(_ui->spinbox_scale->maximum(),value);
        value = std::max<double>(_ui->spinbox_scale->minimum(), value);

        _ui->spinbox_scale->setValue(value);
        scaleChanged(value);
        repaint();
    } else {
        _scroll_parent->verticalScrollBar()->setEnabled(true);
    }
}

void ImageCanvas::keyPressEvent(QKeyEvent * event) {
    if (event->key() == Qt::Key_Enter) {
        //regenerate();


        //emit(_ui->button_smart_mask->released());
    } else if(event->key() == Qt::Key_Delete){
        int idx = getSelectedBox();
        if (idx != -1)
        {
            delete_layer(idx);
            clear_box_selection();
        }
    }
}

//TODO: Optimize

void ImageCanvas::delete_layer(int index)
{
    if (mask_history.size() == 0 || index > mask_history.size() || index < 0)
    {
        return;
    }

    delete_stack.push(mask_history[index]);
    mask_history.erase(mask_history.begin() + index);

    regenerate();
    redrawBoundingBox();
    update();
}

void ImageCanvas::delete_last_layer()
{
    if (mask_history.size() == 0)
    {
        return;
    }

    delete_stack.push(mask_history.back());
    mask_history.pop_back();

    regenerate();
    redrawBoundingBox();
    update();
}

void ImageCanvas::restore_last_layer()
{
    if (delete_stack.empty())
    {
        return;
    }

    _top_mask = delete_stack.top();
    smartMask();
    delete_stack.pop();

    regenerate();
    redrawBoundingBox();
    update();
}

//TODO: Optimize suggestion:
//  Could store images as cv::Mat rather than QImage, would reduce QImage->Mat conversions
void ImageCanvas::regenerate() {
    box_list.clear();
    clearMask();
    // Might have to set image here
    cv::Mat id_mat = qImage2Mat(_mask.id);
    cv::Mat color_mat = qImage2Mat(_mask.color);
    for (auto m : mask_history)
    {
        cv::Mat img_id = qImage2Mat(m.id);
        cv::Mat img_color = qImage2Mat(m.color);

        // Add BBOX

        BoundingBox bbox = findBoundingBox(img_id, _ui->id_labels);
        box_list.push_back(bbox);

        // Layer masks

        for (int i = 0; i < img_id.rows; i++)
        {
            for (int j = 0; j < img_id.cols; j++)
            {
                auto pixel = id_mat.at<cv::Vec3b>(i,j);
                if (pixel[0] == 0 && pixel[1] == 0 && pixel[2] == 0)
                {
                    id_mat.at<cv::Vec3b>(i,j) = img_id.at<cv::Vec3b>(i,j);
                    color_mat.at<cv::Vec3b>(i,j) = img_color.at<cv::Vec3b>(i,j);
                }
            }
        }
    }

    _mask.id = mat2QImage(id_mat);
    _mask.color = mat2QImage(color_mat);
    redrawBoundingBox();
    update();
}

void ImageCanvas::clear_box_selection()
{
    for (auto bbox : box_list)
    {
        bbox.unselect();
    }
}

void ImageCanvas::setImageMask(const ImageMask & mask) {
    _mask = mask;
}


void ImageCanvas::setSmartImageMask(const ImageMask & smart_mask) {
    _smart_mask = smart_mask;
}

void ImageCanvas::setId(int id) {
    _color.id = QColor(id, id, id);
    _cid = id;
    _color.color = _ui->id_labels[id]->color;
}

void ImageCanvas::refresh() {
    update();
}


void ImageCanvas::undo() {
    auto top_mask_mat = qImage2Mat(_top_mask.id);
    cv::Scalar sum = cv::sum(top_mask_mat);
    if (cv::sum(sum)[0] > 0)
    {
        // Pretty slow
        regenerate();
        return;
    }

    delete_layer(mask_history.size() - 1);
    return;
}

void ImageCanvas::redo() {
    restore_last_layer();
    return;
}

std::string ImageCanvas::getObjectString(){
    return _ui->id_labels[_cid]->name.toStdString();
}
