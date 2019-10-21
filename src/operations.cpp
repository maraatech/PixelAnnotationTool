#include "operations.h"
#include <algorithm>

void DrawOperation::undo() {
	auto imask = _canvas->getImageMask();
	auto smart_imask = _canvas->getSmartImageMask();

	imask.setMask(_mask_diff.removeDiff(imask.getMask()));
	smart_imask.setMask(_mask_diff.removeDiff(smart_imask.getMask()));

	_canvas->setImageMask(imask);
	_canvas->setSmartImageMask(smart_imask);
}

void DrawOperation::redo() {
	auto imask = _canvas->getImageMask();
	auto smart_imask = _canvas->getSmartImageMask();

	imask.setMask(_mask_diff.applyDiff(imask.getMask()));
	smart_imask.setMask(_mask_diff.applyDiff(smart_imask.getMask()));

	_canvas->setImageMask(imask);
	_canvas->setSmartImageMask(smart_imask);
}

void SmartMaskOperation::undo() {
    _canvas->setBoxList(_before);
	_canvas->redrawBoundingBox();
}

void SmartMaskOperation::redo() {
    _canvas->setBoxList(_after);
	_canvas->redrawBoundingBox();
}

void CreateBoxOperation::undo() {
	if (_create_flag) {
		_canvas->getBoxList().pop_back();
	} else	{
		_canvas->getBoxList().push_back(_new_bbox);
	}
	_canvas->redrawBoundingBox();
}

void CreateBoxOperation::redo() {
	if (_create_flag) {
		_canvas->getBoxList().push_back(_new_bbox);
	} else {
		_canvas->getBoxList().pop_back();
	}
	_canvas->redrawBoundingBox();
}

void ChangeBoxOperation::undo() {
	_canvas->getBoxList()[_index] = _old_bbox;
	_canvas->redrawBoundingBox();
}

void ChangeBoxOperation::redo() {
	_canvas->getBoxList()[_index] = _new_bbox;
	_canvas->redrawBoundingBox();
}


OperationManager::OperationManager(ImageCanvas * canvas) {
	_canvas = canvas;
	this->sync();
}

void OperationManager::sync() {
	curr_mask = _canvas->getImageMask().getMask().clone();
	curr_smart_mask = _canvas->getSmartImageMask().getMask().clone();
	curr_bbox_list = _canvas->getBoxList();
}

void OperationManager::add_op(Operations * op) {
	op_list.erase(op_list.begin() + _index, op_list.end());
	op_list.push_back(op);
	_index++;

	sync(); // Should sync after every op is made
}

void OperationManager::save_draw() {
	auto mask = _canvas->getImageMask().getMask();
	auto smart_mask = _canvas->getSmartImageMask().getMask();

	MaskDiff mask_diff = MaskDiff(curr_mask, mask);
	MaskDiff smart_mask_diff = MaskDiff(curr_smart_mask, smart_mask);

	this->add_op(new DrawOperation(_canvas, mask_diff, smart_mask_diff));
}

void OperationManager::create_bbox(BoundingBox bbox) {
	this->add_op(new CreateBoxOperation(_canvas, bbox, true));
	bbox.printBoxParam();
}

void OperationManager::delete_bbox(BoundingBox bbox) {
	this->add_op(new CreateBoxOperation(_canvas, bbox, false));
	bbox.printBoxParam();
}

void OperationManager::change_bbox(BoundingBox bbox, int index) {
	this->add_op(new ChangeBoxOperation(_canvas, bbox, curr_bbox_list[index], index));
	bbox.printBoxParam();
}

void OperationManager::smart_mask(const std::vector<BoundingBox> before, const std::vector<BoundingBox> after) {
	this->add_op(new SmartMaskOperation(_canvas, before, after));
}

void OperationManager::undo() {
	std::cout << "Performing Undo!" << std::endl;
	if (_index <= 0) {
		return;
	} else if (_index > op_list.size()) {
		std::cout << "Index has fallen out of sink of op list" << std::endl;
	}
	std::cout << "Op List [" << std::to_string(op_list.size()) << "] Index: " << std::to_string(_index) << std::endl;
	Operations *op = op_list[_index - 1];
	op->speak();
	op->undo();
	_index--;
}

void OperationManager::redo() {
	std::cout << "Performing Redo!" << std::endl;
	if (_index == op_list.size()){
		return;
	} else if (_index < 0) {
		std::cout << "Index has fallen out of sink of op list" << std::endl;
	}

	std::cout << "Op List [" << std::to_string(op_list.size()) << "] Index: " << std::to_string(_index) << std::endl;
	Operations *op = op_list[_index];
	op->speak();
	op->redo();
	_index++;
}
