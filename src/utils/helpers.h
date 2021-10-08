#include <QList>
#include <QSet>

template <typename T, template<typename> typename C>
QSet<T> toQSet(const C<T> &container)
{
  return QSet<T>(container.begin(), container.end());
}

template <typename T, template<typename> typename C>
QList<T> toQList(const C<T> &container)
{
  return QList<T>(container.begin(), container.end());
}

