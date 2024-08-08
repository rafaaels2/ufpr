Rails.application.routes.draw do
  root "home#index"
  get 'admin', to: 'admin#index'
  get 'user', to: 'user#index'
  
  resources :pessoas do
  collection do
      get 'visualizar'
    end
  end

  resources :carros do
    collection do
      get 'visualizar'
    end
  end

  resources :computadors do
    collection do
      get 'visualizar'
    end
  end

  resources :disciplinas do
    collection do
      get 'visualizar'
    end
  end

  resources :disciplinas_pessoas do
    collection do
      get 'visualizar'
    end
  end
end

