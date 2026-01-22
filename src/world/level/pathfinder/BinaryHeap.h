#pragma once

#include "world/level/pathfinder/Node.h"
#include <vector>
#include <memory>
#include <limits>
#include <stdexcept>

// newb12: BinaryHeap - min-heap for A* pathfinding
// Reference: newb12/net/minecraft/world/level/pathfinder/BinaryHeap.java
class BinaryHeap
{
private:
	std::vector<std::shared_ptr<pathfinder::Node>> heap;
	int size = 0;

	void upHeap(int index)
	{
		std::shared_ptr<pathfinder::Node> node = heap[index];
		float cost = node->f;
		
		while (index > 0)
		{
			int parent = (index - 1) >> 1;
			std::shared_ptr<pathfinder::Node> parentNode = heap[parent];
			if (!(cost < parentNode->f))
			{
				break;
			}
			
			heap[index] = parentNode;
			parentNode->heapIdx = index;
			index = parent;
		}
		
		heap[index] = node;
		node->heapIdx = index;
	}

	void downHeap(int index)
	{
		std::shared_ptr<pathfinder::Node> node = heap[index];
		float cost = node->f;
		
		while (true)
		{
			int left = 1 + (index << 1);
			int right = left + 1;
			if (left >= size)
			{
				break;
			}
			
			std::shared_ptr<pathfinder::Node> leftNode = heap[left];
			float leftCost = leftNode->f;
			std::shared_ptr<pathfinder::Node> rightNode = nullptr;
			float rightCost = std::numeric_limits<float>::infinity();
			if (right < size)
			{
				rightNode = heap[right];
				rightCost = rightNode->f;
			}
			
			if (leftCost < rightCost)
			{
				if (!(leftCost < cost))
				{
					break;
				}
				
				heap[index] = leftNode;
				leftNode->heapIdx = index;
				index = left;
			}
			else
			{
				if (!(rightCost < cost))
				{
					break;
				}
				
				heap[index] = rightNode;
				rightNode->heapIdx = index;
				index = right;
			}
		}
		
		heap[index] = node;
		node->heapIdx = index;
	}

public:
	BinaryHeap()
	{
		heap.resize(1024);
	}

	std::shared_ptr<pathfinder::Node> insert(std::shared_ptr<pathfinder::Node> node)
	{
		if (node->heapIdx >= 0)
		{
			throw std::runtime_error("OW KNOWS!");
		}
		
		if (size == heap.size())
		{
			heap.resize(size << 1);
		}
		
		heap[size] = node;
		node->heapIdx = size;
		upHeap(size++);
		return node;
	}

	void clear()
	{
		size = 0;
	}

	std::shared_ptr<pathfinder::Node> peek()
	{
		return heap[0];
	}

	std::shared_ptr<pathfinder::Node> pop()
	{
		std::shared_ptr<pathfinder::Node> node = heap[0];
		heap[0] = heap[--size];
		heap[size].reset();
		if (size > 0)
		{
			downHeap(0);
		}
		
		node->heapIdx = -1;
		return node;
	}

	void remove(std::shared_ptr<pathfinder::Node> node)
	{
		heap[node->heapIdx] = heap[--size];
		heap[size].reset();
		if (size > node->heapIdx)
		{
			if (heap[node->heapIdx]->f < node->f)
			{
				upHeap(node->heapIdx);
			}
			else
			{
				downHeap(node->heapIdx);
			}
		}
		
		node->heapIdx = -1;
	}

	void changeCost(std::shared_ptr<pathfinder::Node> node, float newCost)
	{
		float oldCost = node->f;
		node->f = newCost;
		if (newCost < oldCost)
		{
			upHeap(node->heapIdx);
		}
		else
		{
			downHeap(node->heapIdx);
		}
	}

	int getSize() const { return size; }
	bool isEmpty() const { return size == 0; }
};