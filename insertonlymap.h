/* InsertOnlyMap     Copyright (C) 2016 Frans Faase

   Simple class for a insert-only map.
      
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

GNU General Public License:
   http://www.iwriteiam.nl/GNU.txt

Details:
   http://www.iwriteiam.nl/D1604.html#24

*/

template <class Key, class Value>
class InsertOnlyMap
{
public:
	InsertOnlyMap() : _root(0) {}
	~InsertOnlyMap() { delete _root; }
	Value *findOrCreate(const Key& key)
	{
		Value *value = 0;
		_find_or_create(key, _root, 0, value);
		return value;
	}

private:
	struct tree_t
	{
		tree_t(Key n_key) : key(n_key), value(n_key), left(0), right(0), depth(0) {}
		~tree_t() { delete left; delete right; }
		void calc_depth()
		{
			if (left == 0)
				depth = right == 0 ? 0 : right->depth + 1;
			else if (right == 0)
				depth = left->depth + 1;
			else
				depth = (left->depth > right->depth ? left->depth : right->depth) + 1;
		}
		Key key;
		Value value;
		tree_t* left;
		tree_t* right;
		tree_t* parent; // used by iterator class
		int depth;
	};
public:

	class iterator
	{
	public:
		iterator(const InsertOnlyMap& map)
		{
			_cur = map._root;
			if (_cur == 0)
				return;
			_cur->parent = 0;
			while (_cur->left != 0)
			{
				_cur->left->parent = _cur;
				_cur = _cur->left;
			}
		}
		bool more() const { return _cur != 0; }
		void next()
		{
			if (_cur->right != 0)
			{
				_cur->right->parent = _cur;
				_cur = _cur->right;
				while (_cur->left != 0)
				{
					_cur->left->parent = _cur;
					_cur = _cur->left;
				}
			}
			else
			{
				for(;;)
				{
					tree_t* prev = _cur;
					_cur = _cur->parent;
					if (_cur == 0 || _cur->left == prev)
						break;
				}
			}
		}
		const Key& key() const { return _cur->key; }
		Value& value() const { return _cur->value; }
	private:
		tree_t* _cur;
	};

	tree_t* _root;
private:
	bool _find_or_create(const Key& key, tree_t *&tree, tree_t* parent, Value *&value)
	{
		if (tree == 0)
		{
			tree = new tree_t(key);
			value = &tree->value;
			if (parent != 0)
				parent->calc_depth();
			return true;
		}
		int c = key.compare(tree->key);
		if (c == 0)
			value = &tree->value;
		else if (c < 0)
		{
			if (_find_or_create(key, tree->left, tree, value))
			{
				if (tree->left->depth > (tree->right == 0 ? 1 : tree->right->depth+1))
				{
					tree_t *old_tree = tree;
					tree = tree->left;
					tree_t *middle_tree = tree->right;
					tree->right = old_tree;
					old_tree->left = middle_tree;
					old_tree->calc_depth();
				}
				tree->calc_depth();
				return true;
			}			
		}
		else
		{
			if (_find_or_create(key, tree->right, tree, value))
			{
				if (tree->right->depth > (tree->left == 0 ? 1 : tree->left->depth+1))
				{
					tree_t *old_tree = tree;
					tree = tree->right;
					tree_t *middle_tree = tree->left;
					tree->left = old_tree;
					old_tree->right = middle_tree;
					old_tree->calc_depth();
				}
				tree->calc_depth();
				return true;
			}
		}
		return false;
	}

public:
	bool contains(const Key& key) const
	{
		tree_t* tree = _root;
		while (tree != 0)
		{
			int c = key.compare(tree->key);
			if (c == 0)
				return true;
			tree = c < 0 ? tree->left : tree->right;
		}
		return false;
	}
	Value *find(const Key& key)
	{
		tree_t* tree = _root;
		while (tree != 0)
		{
			int c = key.compare(tree->key);
			if (c == 0)
				return &tree->value;
			tree = c < 0 ? tree->left : tree->right;
		}
		return 0;
	}
};
