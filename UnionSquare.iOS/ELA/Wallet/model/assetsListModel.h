//
//  assetsListModel.h
//  elastos wallet
//
//  Created by 韩铭文 on 2019/1/15.
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface assetsListModel : NSObject
/*
 *<# #>
 */
@property(copy,nonatomic)NSString *iconName;
/*
 *<# #>
 */
@property(copy,nonatomic)NSString *iconBlance;
/*
 *<# #>
 */
@property(copy,nonatomic)NSString *updateTime;
/*
 *<# #>
 */
@property(assign,nonatomic)CGFloat thePercentageCurr;
@property(assign,nonatomic)CGFloat thePercentageMax;
@property(assign,nonatomic)CGFloat thePercentFl;
@end

NS_ASSUME_NONNULL_END
