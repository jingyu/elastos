package org.elastos.wallet.ela.ui.find.fragment;

import android.os.Bundle;
import android.support.v7.widget.LinearLayoutManager;
import android.support.v7.widget.RecyclerView;
import android.view.View;
import android.widget.ImageView;
import android.widget.TextView;

import org.elastos.wallet.R;
import org.elastos.wallet.ela.base.BaseFragment;
import org.elastos.wallet.ela.ui.common.listener.CommonRvListener;
import org.elastos.wallet.ela.ui.crvote.CRListFragment;
import org.elastos.wallet.ela.ui.crvote.bean.FindListBean;
import org.elastos.wallet.ela.ui.find.FindListRecAdapter;
import org.elastos.wallet.ela.ui.vote.SuperNodeListFragment;
import org.elastos.wallet.ela.utils.SPUtil;

import java.util.ArrayList;
import java.util.List;

import butterknife.BindView;

public class FindFragment extends BaseFragment implements CommonRvListener {
    @BindView(R.id.iv_title_left)
    ImageView ivTitleLeft;
    @BindView(R.id.tv_title)
    TextView tvTitle;
    @BindView(R.id.rv)
    RecyclerView rv;
    private FindListRecAdapter adapter;
    private List<FindListBean> list;

    @Override
    protected int getLayoutId() {
        return R.layout.fragment_find;
    }

    @Override
    protected void initInjector() {

    }

    @Override
    protected void initView(View view) {
        tvTitle.setText(getString(R.string.social));
        ivTitleLeft.setVisibility(View.GONE);
        list = new ArrayList<>();
        FindListBean bean1 = new FindListBean();
        bean1.setResouceId(R.mipmap.found_dpos_icon);
        bean1.setUpText(getString(R.string.supernode_election));
        bean1.setDownText(getString(R.string.findlistdown1));
        FindListBean bean2 = new FindListBean();
        bean2.setResouceId(R.mipmap.found_cr_vote);
        bean2.setUpText(getString(R.string.findlistup2));
        bean2.setDownText(getString(R.string.findlistdown2));
        list.add(bean1);
        list.add(bean2);
        //  list.add(R.mipmap.found_card_id);
        //list.add(R.mipmap.found_card_paradrop);
//        presenter = new FindPresenter();
//        presenter.getSupportedChains(wallet.getWalletId(), MyWallet.ELA, this);
        setRecycleView();
    }

    public static FindFragment newInstance() {
        Bundle args = new Bundle();
        FindFragment fragment = new FindFragment();
        fragment.setArguments(args);
        return fragment;
    }

    private void setRecycleView() {
        if (adapter == null) {
            adapter = new FindListRecAdapter(getContext(), list);
            rv.setLayoutManager(new LinearLayoutManager(getContext(), LinearLayoutManager.VERTICAL, false));
            rv.setAdapter(adapter);
            adapter.setCommonRvListener(this);

        } else {
            adapter.notifyDataSetChanged();
        }
    }

    @Override
    public void onRvItemClick(int position, Object o) {
        if (position == 0) {
            ((BaseFragment) getParentFragment()).start(SuperNodeListFragment.class);
        } else if (position == 1) {
            ((BaseFragment) getParentFragment()).start(CRListFragment.class);
        }
    }

    /**
     * 处理回退事件
     *
     * @return
     */
    @Override
    public boolean onBackPressedSupport() {
        return closeApp();
    }


}
